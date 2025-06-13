# Building web apps from scratch - The Listener Socket - Part 5

Now that we have a pretty good handle on how the HTTP protocol works, we can start with the implementation. The way we will go about this is to first set up a plain TCP echo server using system calls and document step by step what everything does (an echo server is one that repeats back any message it receives). Then, we will use this generic TCP server to actually process HTTP requests and serve responses. In this post we will talk about setting up a listening socket.

<br />
<br />

The code snippets used here are Linux-specific. The general points also apply to Windows with some minor fixes I mentioned in the "Windows" paragraph. At the end you will find the program in its entirety in a form that's compatible with both Linux and Windows.

## Creating a Socket

First, we need to create a "socket" object to tell the system we are interested in accepting TCP connections on a given interface and port. This is done using the `socket()` function:

```c
int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
if (listen_fd == -1) {
	return -1;
}
```

the first argument `AF_INET` tells the OS we want our TCP socket to be based on IP version 4. If we wanted to use IP version 6 or bluetooth, we would use a different value here.

<br />
<br />

The second argument `SOCK_STREAM` tells the OS we are going to use the socket interface for stream-oriented protocols.

<br />
<br />

The third argument specifies which stream-oriented protocol we are going to use specifically, or more generally which protocol we are going to use with the second argument's socket type. The value `0` is associated to the default protocol for that socket type, which is TCP in the case of `SOCK_STREAM`. At first I was confused about this and thought the third argument was redundant, but after talking to [Anshuman](https://x.com/AnshumanKhanna5) on the blog's [discord](https://discord.gg/vCKkCWceYP) I realized that wasn't the case!

<br />
<br />

The return value is an integer that identifies the socket object which is stored in the operating system's kernel, or -1 if the function failed.

## Enabling Address Reuse (optional)

This is an optional step helpful while developing servers. When a TCP connection that uses a given local address and port pair is terminated, the actively closing end enters the TIME-WAIT state (see [RFC 9293](https://datatracker.ietf.org/doc/html/rfc9293#name-state-machine-overview)) where no new connections can be established on the interface/port pair to let any residual network traffic related to it settle. This is extremely annoying when developing a server because it means you'll have to wait a couple minutes between runs. If you run the server before then, you'll get an "Address already in use" error. To avoid this, you can set the address reuse option on the socket:

```c
int one = 1;
setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (char*) &one, sizeof(one));
```

This will set the `SO_REUSEADDR` setting to 1 for this socket.

## Binding to the Local Interface

We created a socket, but it's not doing anything yet since we need to configure it first. We can start by telling the OS which interface and port pair we want to listen on. We do so using the `bind()` function.

```c
// We want to listen for connections on this interface and port
char addr[] = "127.0.0.1";
int port = 8080;

struct sockaddr_in bind_buf;
// TODO: Write the address and port into "bind_buf"

// Bind the socket to the specified interface
if (bind(listen_fd, (struct sockaddr*) &bind_buf, sizeof(bind_buf)) < 0) {

	// If an error occurred, close the socket and return
	close(listen_fd);
	return -1;
}
```
This works by filling out the `struct sockaddr_in` structure with our parameters and then pass that to `bind()`. If we were using a different L3 protocol such as IPv6 we would be using a different address structure here.

<br />
<br />

The fields of `struct sockaddr_in` we care about initializing are `sin_family`, `sin_port`, `sin_addr`.  The `sin_family` refers to the underlying level 3 protocol we are using, which is still `AF_INET` for IPv4, while the `sin_port` and `sin_addr` fields contains the port and address we will listen on.

```c
struct sockaddr_in bind_buf;

// L3 protocol
bind_buf.sin_family = AF_INET;

// port
bind_buf.sin_port = htons(port);

// address
if (inet_pton(AF_INET, addr, &bind_buf.sin_addr) != 1) {
	close(listen_fd);
	return -1;
}
```

The way to set the `sin_port` field is by first converting the port value from host byte order to network byte order using the `htons` function. Without going too much into detail, different CPUs store multi-byte data types in different ways. This is referred to as the CPU's "endianess". Due to this difference, if CPUs wrote data to the network as they store it in memory, CPUs with different endianess would receive the proper bytes but translate them logically as a different value. To avoid this issue, whenever we write multi-byte values to the network, we must convert the byte ordering to the network's conventional byte order. The `htons` function does exactly this for 16 bit values.

<br />
<br />

Setting the `sin_addr` field is a bit trickier as we usually specify the IPv4 addresses in dot-decimal form. That's why we rely on the standard `inet_pton` function to convert the string to the raw 4 byte address. The resulting value already has the correct byte ordering here. The first argument `inet_pton` specifies which address format it should parse. For dot-decimal we pass it AF_INET. The second argument is the actual null-terminated string. The last argument is the memory location where the address's 32 bits would be written to. The return value is 1 on success and 0 or -1 on failure.

## Listening for Connections

Now we can "activate" our socket by calling the `listen()` function. This will tell the OS to start performing the three-way handshakes and store the established connections in a backlog. After that we will be able to get connections from that backlog to our application by calling the `accept()` function:

```c
if (listen(listen_fd, 32) < 0) {
	close(listen_fd);
	return -1;
}
```

the first argument is the listening socket, while the second one is the backlog size. If a client tries to establish a TCP connection while the backlog is full, it will be rejected, so we need to choose a backlog size based on how fast our application is at accepting connections. In practice this is rarely a problem. Even if you used the value of 1 you probably wouldn't notice until you stress-tested your server.

## Windows

Windows users will need to set up the process's socket context before any of the socket-related functions are called. This is done by calling `WSAStartup()`:

```c
WSADATA wd;
if (WSAStartup(MAKEWORD(2, 2), &wd))
	return;
```

when the process is done using the network, it should call `WSACleanup`:

```c
WSACleanup();
```

On windows the socket type is not `int` but `SOCKET`, the function for closing sockets it `closesocket`, and the invalid value for a socket handle is not `-1` but `INVALID_SOCKET`.

## Putting Everything Together

Now let's put everything together and include the proper headers:

```c
#ifdef _WIN32
#include <winsock2.h> // WSAStartup, WSACleanup, socket, bind, listen, closesocket
#include <ws2tcpip.h> // inet_pton
#define CLOSE_SOCKET closesocket
#else
#include <unistd.h> // close
#include <sys/socket.h> // socket, bind, listen
#include <arpa/inet.h> // htons
#define SOCKET int
#define INVALID_SOCKET -1
#define CLOSE_SOCKET close
#endif

int main(void)
{
#ifdef _WIN32
	WSADATA wd;
	if (WSAStartup(MAKEWORD(2, 2), &wd))
		return -1;
#endif

	SOCKET listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd == INVALID_SOCKET) {
		return -1;
	}

	int one = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (char*) &one, sizeof(one));

	// We want to listen for connections on this interface and port
	char addr[] = "127.0.0.1";
	int port = 8080;

	struct sockaddr_in bind_buf;
	bind_buf.sin_family = AF_INET;
	bind_buf.sin_port = htons(port);
	if (inet_pton(AF_INET, addr, &bind_buf.sin_addr) != 1) {
		CLOSE_SOCKET(listen_fd);
		return -1;
	}
	if (bind(listen_fd, (struct sockaddr*) &bind_buf, sizeof(bind_buf)) < 0) {
		CLOSE_SOCKET(listen_fd);
		return -1;
	}

	if (listen(listen_fd, 32) < 0) {
		CLOSE_SOCKET(listen_fd);
		return -1;
	}

	// TODO: Accept and process connections


#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}
```

This code will work automatically on Windows and Linux by detecting the platform based on if `_WIN32` is defined or not.

## Building the Project

Let's say the code is in the `main.c` file. To compile the program on Linux, open the terminal and run this command:

```
gcc -o main main.c
```

if you are on Windows, this is the command you must run:

```
gcc -o main.exe main.c -lws2_32
```

the last flag tells the linker we are using the windows socket module.

## What's next
In the next post we'll introduce the accept loop! The loop where our server will spend most of its time accepting incoming connections and processing them.

## Join the Discussion!
Have questions or feedback for me? Feel free to pop in my discord

<br />
<br />

[Join the Discord Server](https://discord.gg/vCKkCWceYP)