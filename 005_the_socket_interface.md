# Building web apps from scratch - The Socket Interface - 
Part 1

**NOTE: This post is still a work in progress!!**

At the lowest level, our web app uses the TCP protocol for transmitting data over the network.

The Transmission Control Protocol (TCP) allows us to send between hosts in a network a stream of bytes. All bytes we send will arrive in order. There is no concept of a "message". The bytes are not grouped logically in any way. There is only a sequence of bytes.

At this level we need to manage the connections used to send these stream of bytes. We will then need to implement the HTTP protocol to group these bytes into HTTP requests and responses.

The TCP is implemented for us by the OS, which then allows us to control it through the socket interface. A socket is a kernel object. It's a bit hard to describe what a socket represents exactly. Each connection is described by a socket, but we also have a different type a socket to establish connections. I usually refer to them as "listener socket" and "connection socket".

First of all, we need to create a plain socket object. We do so with the `socket` function
```
int socket_handle = socket(AF_INET, SOCK_STREAM, 0);
```
The `AF_INET` argument tells the system we want the socket to use `IPv4`. If we passed in `AF_INET6` we would use `IPv6`.

The `SOCK_STREAM` argument tells we want to use TCP. Sockets may be used for other protocols too.

The return value is an integer which identifies our socket object. The integer, usually referred to as handle, must be non-negative to be valid. If a negative value is returned, an error occurred.

Now we must configure this socket object as a listener socket. We do so with the `bind` function.

```c
struct sockaddr_in bind_buffer;
bind_buffer.sin_family = AF_INET;
bind_buffer.sin_port   = htons(8080);
bind_buffer.sin_addr.s_addr = inet_addr("127.0.0.1");

if (bind(socket_handle, (struct sockaddr*) &bind_buffer, sizeof(bind_buffer))) {
	// An error occurred
}
```
This function allows us to specify from which network interface the socket can accept connections. Lets say our computer has two network interfaces, each one partecipating in a different network. Each interface will have its own IP address, say, `192.168.0.3` and `10.0.0.4`. If we only want to talk to computers to the first network, we would specify `192.168.0.3` to `bind`.

In this example, we specified `127.0.0.1`. This is a special address that identifier the loopback network interface. This is a virtual interface used used by processes within the same machine to talk to each other. We will use this at first since we will only test the server locally.

There is also a special value called `INADDR_ANY` which tells the system we want to listen from all network interfaces. We will probably use this by the end.

this is definitely the weirdest function. First of all, the configuration happens by filling up an auxiliary structure called `struct sockaddr_in`. Each 