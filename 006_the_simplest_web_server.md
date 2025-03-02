# Building web apps from scratch - The Simplest Web Server - Part 1

**NOTE: This post is still a work in progress!!**

As we mentioned in the last post, the first layer of our web application is the TCP layer. Here we'll start by listening for new client connections and handle incoming data, which is in the form of a byte stream. Later, we'll use this raw communication layer to create the HTTP request-response abstraction.

We will first start with the Linux code, and then adapt it to work on Windows. Also, don't worry if you're missing some details, I'll add the complete program at the end!

To handle these connections we need to create a "socket". We use a socket to define which interface we are listening on, and one extra socket per client connection. Sockets are just the way we tell the OS how we want to manage things. First, we start from the listening socket:

```c
// Create the listening socket
int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
if (listen_socket == -1) { /* error */ }
```

now, we specify which interface we want to listen on. We do this by filling the `struct sockaddr_in` structure and associating it to the socket with the `bind` call

```c
struct sockaddr_in bind_buffer;
bind_buffer.sin_family = AF_INET;
bind_buffer.sin_port = htons(8080);
bind_buffer.sin_addr = inet_addr("127.0.0.1");

if (bind(listen_socket, (struct sockaddr*) &bind_buffer, sizeof(bind_buffer))) {
  /* error */
}
```

this tells the system we want to listen on interface 127.0.0.1 and port 8080. The 127.0.0.1 refers to the loopback interface, which is used by processes to talk to other processes. In other words this allows us to test the server without being open to the local network. When we put the server online, this will need to change. The port used by HTTP is 80, and the one used by HTTPS is 443. We'll have to change that too later.

Now we tell the system to actually use the socket and listen for connections:

```c
if (listen(listen_socket, 32)) { /* error */ }
```

the second argument is the size of the backlog. When connection requests first arrive to the system, they are inserted in a queue. Our process will then use the `accept` function to get a connection from that queue. The backlog parameters tells the system how long we want that queue. This parameter is important because when the queue is full other incoming connections will be rejected. But in general this isn't much of a problem since the server easily keeps up.

Now comes the main loop of the server, where the great majority of its time will be spent. Each iteration will accept a connection, receive a request, then send a response. Lets see:

```c
while (1) {
  int client_socket = accept(listen_socket, NULL, NULL);
  if (client_socket == -1) { /* error */ }

  char request_buffer[4096];
  int len = recv(client_socket, request_buffer, sizeof(request_buffer), 0);
  if (len < 0) { /* error */ }

  // We ignore the request contents for now and always
  // respond with the same message
  char response_buffer[] =
    "HTTP/1.0 200 OK\r\n"
    "Content-Length: 13\r\n"
    "Content-Type: text/plain\r\n"
    "\r\n"
    "Hello, world!";
  send(client_socket, response_buffer, sizeof(response_buffer), 0);  

  close(client_socket);
}
```

Perfect! This is extremely bare-bones but should work. Here is the complete version of the program:

```c
#include <stdio.h> // printf
#include <unistd.h> // close
#include <arpa/inet.h> // socket, htons, inet_addr, sockaddr_in, bind, listen, accept, recv, send

int main()
{
  // Create the listening socket
  int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_socket == -1) {
    printf("socket failed\n");
    return -1;
  }

  struct sockaddr_in bind_buffer;
  bind_buffer.sin_family = AF_INET;
  bind_buffer.sin_port = htons(8080);
  bind_buffer.sin_addr = inet_addr("127.0.0.1");

  if (bind(listen_socket, (struct sockaddr*) &bind_buffer, sizeof(bind_buffer))) {
    printf("bind failed\n");
    return -1;
  }

  if (listen(listen_socket, 32)) {
    printf("listen failed\n");
    return -1;
  }

  while (1) {
    int client_socket = accept(listen_socket, NULL, NULL);
    if (client_socket == -1) {
      printf("accept failed\n");
      continue;
    }

    char request_buffer[4096];
    int len = recv(client_socket, request_buffer, sizeof(request_buffer), 0);
    if (len < 0) {
      printf("recv failed\n");
      close(client_socket);
      continue;
    }

    // We ignore the request contents for now and always
    // respond with the same message
    char response_buffer[] =
      "HTTP/1.0 200 OK\r\n"
      "Content-Length: 13\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "Hello, world!";
    send(client_socket, response_buffer, sizeof(response_buffer), 0);

    close(client_socket);
  }
  // This point will never be reached
}
```

For the windows version, there are a number of minor differences:
1. We must initialize the socket subsystem by calling `WSAStartup`
1. The socket type is `SOCKET` instead of `int`
1. The invalid value for a socket is `INVALID_SOCKET` instead of `-1`
1. Instead of `close` we use `closesocket`
1. The headers to include

To make a cross-platform version, we can leverage the compiler's preprocessor. If the `_WIN32` symbol is defined, we know we are on windows. You can find the full code [here](../sourcecode/part1.c).

it’s a bit ugly, but fortunately we won’t need to do this often. Save this program to a `http_server.c` file and open a terminal in the same directory, then compile the program by running this on Linux:

```sh
gcc http_server.c -o http_server.out
```

and this on windows:

```sh
gcc http_server.c -o http_server.out -lws2_32
```

Now open a browser and visit [http://127.0.0.1:8080/](http://127.0.0.1:8080/). You should be a page saying "Hello, world!".