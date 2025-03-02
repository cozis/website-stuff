# Building web apps from scratch - Request Parsing - Part 2

**NOTE: This post is still a work in progress!!**

Our little web server is very cool, isn't it? The missing thing now, from a feature stand point, is sending a response based on what the client requested. But first, we need to improve its robustness a bit. There are some error corner cases that we haven't handled yet.

### Handling partial sends

The first issue is relative to recv and send. We assumed that when we pass a buffer to send, the system will send the entire message or fail. This isn't correct. This function can also do partial sends. Lets say we want to send 100 bytes. The first time we send only 10 bytes may be sent, so we need to call send again with the remaining bytes

```c
int send_all(SOCKET_TYPE sock, void *src, size_t num)
{
  size_t sent = 0;
  while (sent < num) {
    int just_sent = send(sock, (char*) src + sent, num - sent, 0);
    if (just_sent < 0) return -1;
    sent += (size_t) just_sent;
  }
  return sent;
}
```

This function calls `send` multiple times until every byte has been sent. If an error occurs before sending all bytes, it returns early with -1. By using this function in place of `send` we made our server much more reliable!

The `recv` function behaves in a similar way. Instead of worrying about sending only part of the bytes we need to worry about not receiving exactly how many bytes we had in our buffer. But since we are ignoring the incoming message for now, it does not make a difference.

### The syntax of an HTTP request

Now we are ready to parse the client's request. This will allow us to return different responses based on what the client sent us.

The proper way to go about this would be reading the [HTTP specification](https://datatracker.ietf.org/doc/html/rfc2616) and implement the parser accordingly, but starting with an approximation is good enough for now.

Here's an example of HTTP request:

```
GET / HTTP/1.1
Host: 127.0.0.1:8080
Connection: keep-alive
sec-ch-ua: "Not(A:Brand";v="99", "Brave";v="133", "Chromium";v="133"
sec-ch-ua-mobile: ?0
sec-ch-ua-platform: "Windows"
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/133.0.0.0 Safari/537.36
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8
Sec-GPC: 1
Accept-Language: en-US,en;q=0.6
Sec-Fetch-Site: none
Sec-Fetch-Mode: navigate
Sec-Fetch-User: ?1
Sec-Fetch-Dest: document
Accept-Encoding: gzip, deflate, br, zstd
```

I obtained this by simply adding a print statement in our server after `recv` (don't forget the null terminator!).

The first line contains three things:
1. method
1. resource path
1. HTTP version
The method changes how the request is handled. The most common methods are
1. GET: The client is asking us for a resource
1. POST: The client is sending us some data
this is a bit simplistic, but we can focus on this later.

You can think of a resource path as the file name the client is requesting. Though the resource may be something other than a file, for instance we could have a resource called `/users` which returns a list of users returned by the database.

The version we'll support for now is `HTTP/1.0`, but this does not matter as version 1 and 1.1 use the same syntax.

The first line terminates with a new line, which is made with a carriage return and newline character. In C, these characters are written as `\r\n`. After that, we have a list of headers, strings in the form:

```
name: value\r\n
```

we can mostly ignore these and only add support for specific ones as we need them. The one we are interested in is the `Content-Length` header, which tells us the length in bytes of the request payload. `GET` requests don't have a payload, so this header is not used. We will need it for `POST` requests.

After the entire header list, we find a `\r\n` character, not considering the `\r\n` used to close the last header. If the request had a payload, this is where it would start.

Just, to give you an example, this is how a `POST` request would look like:

```
POST /some-resource HTTP/1.0\r\n
host: 127.0.0.1\r\n
Content-Length: 15\r\n
\r\n
I'm the payload
```

### Parsing a request

Now we start writing the parser. There are many ways one can go about parsing, This is the way I found is easier for HTTP. The basic idea is we have a pointer to the request bytes, a length, and a cursor. We read the bytes by advancing the cursor and extract information from it by adding it to a structure. When the cursor reaches the end, we have a structure with all the information from the request bytes, easily accessible for our server. The request structure looks like this:

```c
typedef struct { char *data; int size; } string;

#define MAX_HEADERS 32

typedef enum {
  HTTP_METHOD_GET,
  HTTP_METHOD_POST,
} HTTPMethod;

typedef struct {
  string name;
  string value;
} HTTPHeader;

typedef struct {
  int major;
  int minor;
} HTTPVersion;

typedef struct {
  HTTPMethod method;
  string resource_path;
  HTTPVersion version;
  HTTPHeader headers[MAX_HEADERS];
  int num_headers;
} HTTPRequest;
```

Notice how we created a helper structure for strings. This will come in handy for the entire project!

The parsing function will use this interface:

```c
bool parse_request(string src, HTTPRequest *dst)
{
  int cur = 0;

  // .. parsing code goes here ..
}
```

if we parsed the request successfully, true is returned. If some error occurred, then false is returned. When the we succede, the `dst` argument will be initialized.

First, we parse the method. If the request doesn't start with `GET` or `POST`, we consider it an error:

```c
if (3 < src.size
  && src.data[0] == 'G'
  && src.data[1] == 'E'
  && src.data[2] == 'T'
  && src.data[3] == ' ') {
  dst->method = HTTP_METHOD_GET;
  cur = 4;
} else if (4 < src.size
  && src.data[0] == 'P'
  && src.data[1] == 'O'
  && src.data[2] == 'S'
  && src.data[3] == 'T'
  && src.data[4] == ' ') {
  dst->method = HTTP_METHOD_POST;
  cur = 5;
} else {
  // Invalid method
  return false;
}
```

After this block, the cursor will point to the character that comes after the the first space, so the first character of the request path. The path goes from the first space to the second space.

```c
// Check that there is at least one non-space character where the cursor points
if (cur == src.size || src.data[cur] == ' ')
  return false; // No path

// Save the offset of the path in the string
int path_offset = cur;

// The first character is not a space. Now loop until we find one
do
  cur++;
while (cur < src.size && src.data[cur] != ' ');

// There are two ways we exit the loop:
//   1) The cursor reached the end of the string because no space
//      was found (cur == src.size)
//   2) We found a space (src.data[cur] == ' ')
// Of course (1) is an error

if (cur == src.size)
  return false;

int path_length = cur - path_offset;

// Consume the space that comes after the path
cur++;

dst->resource_path = (string) { .data = src.data + path_offset, path_length };
```

Instead creating a copy of the resource path to store it in the `HTTPRequest` structure, we created a string that pointed inside the input buffer. With this trick we avoided a dynamic allocation. The downside of this is that the contents of `HTTPRequest` now depend on the input buffer staying around.

Now we parse the version. We expect one of the following strings: `HTTP/1`, `HTTP/1.0`, `HTTP/1.1`, followed by `\r\n`

```c
// If we don't find the string "HTTP/", that's an error
 if (4 >= src.size - cur
  || src.data[cur+0] != 'H'
  || src.data[cur+1] != 'T'
  || src.data[cur+2] != 'T'
  || src.data[cur+3] != 'P'
  || src.data[cur+4] != '/')
  return false;
cur += 5;

// Now we expect either "1\r\n", "1.0\r\n", or "1.1\r\n"
if (4 < src.size - cur
  && src.data[cur+0] == '1'
  && src.data[cur+1] == '.'
  && src.data[cur+2] == '1'
  && src.data[cur+3] == '\r'
  && src.data[cur+4] == '\n') {
  cur += 5;
  dst->version = (HTTPVersion) {1, 1};
} else if (4 < src.size - cur
  && src.data[cur+0] == '1'
  && src.data[cur+1] == '.'
  && src.data[cur+2] == '0'
  && src.data[cur+3] == '\r'
  && src.data[cur+4] == '\n') {
  cur += 5;
  dst->version = (HTTPVersion) {1, 0};
} else if (2 < src.size - cur
  && src.data[cur+0] == '1'
  && src.data[cur+1] == '\r'
  && src.data[cur+2] == '\n') {
  cur += 3;
  dst->version = (HTTPVersion) {1, 0};
} else {
  // Invalid version
  return false;
}
```

And that was the first line. Now comes the easy part! Now the cursor points to the first character of the list of headers. We must consume headers until we find the final `\r\n` which denotes the end of the request head.

```c
// Initialize the header array
dst->num_headers = 0;

// Loop until we find the final \r\n
while (1 >= src.size - cur
  || src.data[cur+0] != '\r'
  || src.data[cur+1] != '\n') {

  // The cursor now points to the first character of the header's name
  int name_offset = cur;

  // Consume characters until we get to the separator
  while (cur < src.size && src.data[cur] != ':')
    cur++;
  if (cur == src.size)
    return false; // Cursor reached the end of the string
  string header_name = { src.data + name_offset, cur - name_offset };
  cur++; // Consume the ':'

  // Now the cursor points to the first character of the header value
  int value_offset = cur;
  while (cur < src.size && src.data[cur] != '\r')
    cur++;
  if (cur == src.size)
    return false; // Didn't find a '\r'
  string header_value = { src.data + value_offset, cur - value_offset };

  // Now we expect \r\n to terminate the header
  if (1 >= src.size - cur
    || src.data[cur+0] != '\r'
    || src.data[cur+1] != '\n')
    return false; // Didn't find \r\n
  cur += 2;

  if (dst->num_headers == MAX_HEADERS)
    return false; // We reached the end of the static array
  dst->headers[dst->num_headers++] = (HTTPHeader) { header_name, header_value };
}

// We exited the loop, so we know there is a final \r\n we must skip
cur += 2;

// Finished
return true;
```

And there it is! The request parser! It may seem quite daunting, but it's the same trick repeated over and over.

Here is the server with the added parser:

```c
#include <stdio.h> // printf
#include <stdbool.h> // bool, true, false

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

#define MAX_HEADERS 32

typedef enum {
  HTTP_METHOD_GET,
  HTTP_METHOD_POST,
} HTTPMethod;

typedef struct {
  string name;
  string value;
} HTTPHeader;

typedef struct {
  int major;
  int minor;
} HTTPVersion;

typedef struct {
  HTTPMethod method;
  string resource_path;
  HTTPVersion version;
  HTTPHeader headers[MAX_HEADERS];
  int num_headers;
} HTTPRequest;

bool parse_request(string src, HTTPRequest *dst)
{
  int cur = 0;

  if (3 < src.size
    && src.data[0] == 'G'
    && src.data[1] == 'E'
    && src.data[2] == 'T'
    && src.data[3] == ' ') {
    dst->method = HTTP_METHOD_GET;
    cur = 4;
  } else if (4 < src.size
    && src.data[0] == 'P'
    && src.data[1] == 'O'
    && src.data[2] == 'S'
    && src.data[3] == 'T'
    && src.data[4] == ' ') {
    dst->method = HTTP_METHOD_POST;
    cur = 5;
  } else {
    // Invalid method
    return false;
  }

  // Check that there is at least one non-space character where the cursor points
  if (cur == src.size || src.data[cur] == ' ')
    return false; // No path

  // Save the offset of the path in the string
  int path_offset = cur;

  // The first character is not a space. Now loop until we find one
  do
    cur++;
  while (cur < src.size && src.data[cur] != ' ');

  // There are two ways we exit the loop:
  //   1) The cursor reached the end of the string because no space
  //      was found (cur == src.size)
  //   2) We found a space (src.data[cur] == ' ')
  // Of course (1) is an error

  if (cur == src.size)
    return false;

  int path_length = cur - path_offset;

  // Consume the space that comes after the path
  cur++;

  dst->resource_path = (string) { .data = src.data + path_offset, path_length };

  // If we don't find the string "HTTP/", that's an error
  if (4 >= src.size - cur
    || src.data[cur+0] != 'H'
    || src.data[cur+1] != 'T'
    || src.data[cur+2] != 'T'
    || src.data[cur+3] != 'P'
    || src.data[cur+4] != '/')
    return false;
  cur += 5;

  // Now we expect either "1\r\n", "1.0\r\n", or "1.1\r\n"
  if (4 < src.size - cur
    && src.data[cur+0] == '1'
    && src.data[cur+1] == '.'
    && src.data[cur+2] == '1'
    && src.data[cur+3] == '\r'
    && src.data[cur+4] == '\n') {
    cur += 5;
    dst->version = (HTTPVersion) {1, 1};
  } else if (4 < src.size - cur
    && src.data[cur+0] == '1'
    && src.data[cur+1] == '.'
    && src.data[cur+2] == '0'
    && src.data[cur+3] == '\r'
    && src.data[cur+4] == '\n') {
    cur += 5;
    dst->version = (HTTPVersion) {1, 0};
  } else if (2 < src.size - cur
    && src.data[cur+0] == '1'
    && src.data[cur+1] == '\r'
    && src.data[cur+2] == '\n') {
    cur += 3;
    dst->version = (HTTPVersion) {1, 0};
  } else {
    // Invalid version
    return false;
  }

  // Initialize the header array
  dst->num_headers = 0;

  // Loop until we find the final \r\n
  while (1 >= src.size - cur
    || src.data[cur+0] != '\r'
    || src.data[cur+1] != '\n') {

    // The cursor now points to the first character of the header's name
    int name_offset = cur;

    // Consume characters until we get to the separator
    while (cur < src.size && src.data[cur] != ':')
      cur++;
    if (cur == src.size)
      return false; // Cursor reached the end of the string
    string header_name = { src.data + name_offset, cur - name_offset };
    cur++; // Consume the ':'

    // Now the cursor points to the first character of the header value
    int value_offset = cur;
    while (cur < src.size && src.data[cur] != '\r')
      cur++;
    if (cur == src.size)
      return false; // Didn't find a '\r'
    string header_value = { src.data + value_offset, cur - value_offset };

    // Now we expect \r\n to terminate the header
    if (1 >= src.size - cur
      || src.data[cur+0] != '\r'
      || src.data[cur+1] != '\n')
      return false; // Didn't find \r\n
    cur += 2;

    if (dst->num_headers == MAX_HEADERS)
      return false; // We reached the end of the static array
  dst->headers[dst->num_headers++] = (HTTPHeader) { header_name, header_value };
  }

  // We exited the loop, so we know there is a final \r\n we must skip
  cur += 2;

  return true;
}

int send_all(SOCKET_TYPE sock, void *src, size_t num)
{
  size_t sent = 0;
  while (sent < num) {
    int just_sent = send(sock, (char*) src + sent, num - sent, 0);
    if (just_sent < 0) return -1;
    sent += (size_t) just_sent;
  }
  return sent;
}

int main()
{
#ifdef _WIN32
  WSADATA wsaData;
  int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (err != 0) {
    printf("WSAStartup failed\n");
    return -1;
  }
#endif

  // Create the listening socket
  SOCKET_TYPE listen_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_socket == INVALID_SOCKET_VALUE) {
    printf("socket failed\n");
    return -1;
  }

  struct sockaddr_in bind_buffer;
  bind_buffer.sin_family = AF_INET;
  bind_buffer.sin_port = htons(8080);
  bind_buffer.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (bind(listen_socket, (struct sockaddr*) &bind_buffer, sizeof(bind_buffer))) {
    printf("bind failed\n");
    return -1;
  }

  if (listen(listen_socket, 32)) {
    printf("listen failed\n");
    return -1;
  }

  while (1) {
    SOCKET_TYPE client_socket = accept(listen_socket, NULL, NULL);
    if (client_socket == INVALID_SOCKET_VALUE) {
      printf("accept failed\n");
      continue;
    }

    char request_buffer[4096];
    int len = recv(client_socket, request_buffer, sizeof(request_buffer), 0);
    if (len < 0) {
      printf("recv failed\n");
      CLOSE_SOCKET(client_socket);
      continue;
    }

    HTTPRequest parsed_request;
    if (!parse_request((string) {request_buffer, len}, &parsed_request)) {
      // Parsing failed
      char response_buffer[] =
        "HTTP/1.0 400 Bad Request\r\n"
        "Content-Length: 0\r\n"
        "\r\n";
      send_all(client_socket, response_buffer, sizeof(response_buffer));

    } else {
      // Parsing succeded
      char response_buffer[] =
        "HTTP/1.0 200 OK\r\n"
        "Content-Length: 13\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello, world!";
      send_all(client_socket, response_buffer, sizeof(response_buffer));
    }
    CLOSE_SOCKET(client_socket);
  }
  // This point will never be reached
}
```

### Handle partial reads

As we did with send, we need to make sure our call to recv read the entire request head. It is possible we read only part of the request head, in which case we need to call the recv function again. We can stop when we find the `\r\n\r\n`, which signifies the end of the head and start of the body. If we fill up the buffer before we find such token, we consider that an error.

```c
int recv_request_head(SOCKET_TYPE sock, char *dst, int max, int *head_len)
{
  int received = 0;
  while (1) {
    int just_received = recv(sock, dst + received, max - received, 0);
    if (just_received < 0) return -1;
    received += just_received;
    
    // Look for \r\n\r\n
    int i = 0;
    while (3 < received - i
      && (dst[i+0] != '\r'
      || dst[i+1] != '\n'
      || dst[i+2] != '\r'
      || dst[i+3] != '\n'))
      i++;
    if (3 < received - i) {
      // We found the \r\n\r\n and it is at position "i"
      // We consider the head to go from the first byte to the last \n
      *head_len = i + 4;
      break;
    }
    // We did not find the end of the head. If the buffer is now full that's an error
    if (received == max)
      return -1;
  }
  // If we are here we received the head. Note that we may have also read some bytes after the \r\n\r\n, which are part of the request body.
  return received;
}
```

With this, our main loop changes:

```c
// ... parsing and everything else stays the same ...

int main()
{
  // ... this code stays the same too ...

  while (1) {
    SOCKET_TYPE client_socket = accept(listen_socket, NULL, NULL);
    if (client_socket == INVALID_SOCKET_VALUE) {
      printf("accept failed\n");
      continue;
    }

    char request_buffer[4096];
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
      // ... unchanged ...
    } else {
      // ... unchanged ...
    }
    CLOSE_SOCKET(client_socket);
  }
  // This point will never be reached
}
```

### Further improvements
This parser will work well most of the time, but there are a couple corner cases we haven't handled:
* We are blindly accepting characters while parsing the headers and path. These strings have specific syntaxes and allowed characters
* There is something called a "folded header", which is a header spanning over multiple lines. We don't need to handle them, but recognize and reject them at least.