# Building web apps from scratch - Handling request payload - Part 3

**NOTE: This post is still a work in progress!!**

Our server can now receive requests and inspect their data, then send a response based on that data. That's pretty much all we need to get a simple website going, even with dynamic content and everything! There is one thing missing though. At the moment we are ignoring the request payload. This is necessary when the user want to send us information such as login data or a post's content.

At the moment, our server receives the request in a 4Kb buffer, hoping it is large enough to hold its head. We continue reading bytes into the buffer until we find the `\r\n\r\n` token which signifies the end of the request head and the start of the body, if present. It is also possible that our buffer reads over the request head limit and into the body.

Once we receive the head (and some bytes of the body, potentially), we can parse it and read the `Content-Length` header, which will tell us how many bytes we still need to read after the head.

Lets make a function that detects the payload length given the parsed request:

```c
#define NULLSTR ((string) {NULL, 0})
#define S(x) ((string) {(x), (int) sizeof(x)-1})

bool is_digit(char c)
{
  return c >= '0' && c <= '9';
}

char to_lower(char c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';
  return c;
}

bool streq_case_insensitive(string s1, string s2)
{
  if (s1.size != s2.size)
    return false;
  for (int i = 0; i < s1.size; i++)
    if (to_lower(s1.data[i]) != to_lower(s2.data[i]))
      return false;
  return true;
}

int get_content_length(HTTPRequest *request)
{
  string content_length_value = NULLSTR;
  for (int i = 0; i < request->num_headers; i++) {
    if (streq_case_insensitive(S("Content-Length"), request->headers[i].name)) {
      content_length_value = request->headers[i].value;
      break;
    }
  }
  // If we didn't find the header, then the value is set to the empty string, which works too

  // The number may be preceded by spaces
  int cur = 0;

  // Move cursor over the initial spaces
  while (cur < content_length_value.size && content_length_value.data[cur] == ' ')
    cur++;

  // Check that the cursor now points to a number
  if (cur == content_length_value.size)
    return 0; // Empty or missing Content-Length
  if (!is_digit(content_length_value.data[cur]))
    return -1; // Invalid char
 
  // Parse the number for text to integer form
  int content_length = 0;
  do {
    int n = content_length_value.data[cur++] - '0';
    if (content_length > (INT_MAX - n) / 10)
      return -1; // The parsed integer can't be represented by an "int"
    content_length = content_length * 10 + n;
  } while (cur < content_length_value.size && is_digit(content_length_value.data[cur]));

  // We ignore anything that comes after the integer

  return content_length;
}
```

This function returns the payload length or -1 if anything bad happened.

After parsing the head, we read the content length and then call `recv` until we get that many bytes. We also need to handle partial reads, so we implement a `recv_all` analogous to the `send_all` we used some time ago

```c
int recv_all(SOCKET_TYPE sock, char *dst, int num)
{
  int received = 0;
  while (received < num) {
    int just_received = recv(sock, dst + received, num - received, 0);
    if (just_received < 0)
      return -1;
    received += just_received;
  }
  return received;
}
```

this function won't return until exactly num bytes are read, or an error occurres.

This is how the program looks now:

```c
#include <stdio.h> // printf
#include <stdbool.h> // bool, true, false
#include <stdlib.h> // malloc, free
#include <string.h> // memcpy
#include <limits.h> // INT_MAX

#ifdef _WIN32
#include <winsock2.h>
#define SOCKET_TYPE SOCKET
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#define CLOSE_SOCKET closesocket
#else
#include <unistd.h> // close
#include <arpa/inet.h> // socket, htons, inet_addr, sockaddr_in, bind, listen, accept, recv, send
#define SOCKET_TYPE int
#define INVALID_SOCKET_VALUE -1
#define CLOSE_SOCKET close
#endif

typedef struct { char *data; int size; } string;
#define NULLSTR ((string) {NULL, 0})
#define S(x) ((string) {(x), (int) sizeof(x)-1})

bool is_digit(char c)
{
  return c >= '0' && c <= '9';
}

char to_lower(char c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';
  return c;
}

bool streq_case_insensitive(string s1, string s2)
{
  if (s1.size != s2.size)
    return false;
  for (int i = 0; i < s1.size; i++)
    if (to_lower(s1.data[i]) != to_lower(s2.data[i]))
      return false;
  return true;
}

// ... definitions for HTTPRequest, HTTPVersion, HTTPHeader, HTTPMethod ...

bool parse_request(string src, HTTPRequest *dst)
{
  // ...
}

int get_content_length(HTTPRequest *request)
{
  // ...
}

int send_all(SOCKET_TYPE sock, void *src, size_t num)
{
  // ...
}

int recv_all(SOCKET_TYPE sock, char *dst, int num)
{
  // ...
}

int recv_request_head(SOCKET_TYPE sock, char *dst, int max, int *head_len)
{
  // ...
}

int main()
{
  // .. socket, bind, listen ...

  while (1) {
    SOCKET_TYPE client_socket = accept(listen_socket, NULL, NULL);
    if (client_socket == INVALID_SOCKET_VALUE) {
      printf("accept failed\n");
      continue;
    }

    char request_buffer[1024];
    int received_total, head_len;
    received_total = recv_request_head(client_socket, request_buffer, sizeof(request_buffer), &head_len);
    if (received_total < 0) {
      printf("recv_request_head failed\n");
      CLOSE_SOCKET(client_socket);
      continue;
    }
    string request_head = {request_buffer, head_len};

    HTTPRequest parsed_request;
    if (!parse_request(request_head, &parsed_request)) {
      // Parsing failed
      char response_buffer[] =
        "HTTP/1.0 400 Bad Request\r\n"
        "Content-Length: 0\r\n"
        "\r\n";
      send_all(client_socket, response_buffer, sizeof(response_buffer));
      CLOSE_SOCKET(client_socket);
      continue;
    }

    int content_length = get_content_length(&parsed_request);
    if (content_length < 0) {
      char response_buffer[] =
        "HTTP/1.0 400 Bad Request\r\n"
        "Content-Length: 0\r\n"
        "\r\n";
      send_all(client_socket, response_buffer, sizeof(response_buffer));
      CLOSE_SOCKET(client_socket);
      continue;
    }

    string content = NULLSTR;
    if (content_length > 0) {
      content.data = malloc(content_length+1);
      content.size = content_length;

      int received_body = received_total - head_len;
      if (received_body > content_length)
        received_body = content_length;

      memcpy(content.data, request_buffer + head_len, received_body);

      if (received_body < content_length) {
        int result = recv_all(client_socket, content.data + received_body, content.size - received_body);
        if (result < 0) {
          CLOSE_SOCKET(client_socket);
          free(content.data);
          continue;
        }
      }
      content.data[content.size] = '\0';
    }

    // The request payload is in the "content" string

    char response_buffer[] =
      "HTTP/1.0 200 OK\r\n"
      "Content-Length: 13\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "Hello, world!";
    send_all(client_socket, response_buffer, sizeof(response_buffer));

    // Free the dynamic buffer
    free(content.data);

    CLOSE_SOCKET(client_socket);
  }
  // This point will never be reached
}
```

Once the content length is known, we allocate a buffer of the appropriate size with `malloc`. We then receive the body into this buffer with `recv_all`. We ignore the request head and body for now since we always return the usual “Hello, world!” message. We also need to return the memory allocated with `malloc` to the system by calling `free` at the end.