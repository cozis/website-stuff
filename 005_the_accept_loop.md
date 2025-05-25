First of all, let's set up a TCP server that accepts new incoming connections on the loopback interface and port 8080.

```c
#include <sys/socket.h>
#include <arpa/inet.h>

int main(void)
{
	char addr[] = "127.0.0.1";
	unsigned short port = 8080;

	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd == -1) {
		return -1;
	}

	struct sockaddr_in bind_buf;
	bind_buf.sin_family = AF_INET;
	bind_buf.sin_port   = htons(port);
	memset(&bind_buf.sin_zero, 0, sizeof(bind_buf.sin_zero));
	if (inet_pton(AF_INET, addr, &bind_buf.sin_addr) != 1) {
		close(listen_fd);
		return -1;
	}

	if (bind(listen_fd, (struct sockaddr*) &bind_buf, sizeof(bind_buf)) < 0) {
		close(listen_fd);
		return -1;
	}

	if (listen(listen_fd, 32) < 0) {
		close(listen_fd);
		return -1;
	}

	for (;;) {

		int accepted_fd = accept(listen_fd, NULL, NULL);
		if (accepted_fd < 0)
			continue;

		printf("Incoming connection!\n");

		// TODO: The HTTP server logic goes here

		close(accepted_fd);
	}

	return 0;
}
```

that's quite a bit of information in one go, but I figured it would be good to have a look at the entire thing before unpacking each function.

First of all, we create a socket object that we will use to accept new incoming connections:

```c
int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
if (listen_fd == -1) {
	return -1;
}
```

the first argument `AF_INET` tells the OS we want our TCP socket to be based on IP version 4. If we wanted to use IP version 6, we would use AF_INET6 here. The second argument `SOCK_STREAM` tells the OS we are going to use a stream-oriented protocol, or in other terms TCP. I'm not sure what the third argument does, but 0 is the default argument so we can use that. The return value is an integer with describes the socket object which is stored in the operating system's kernel, or -1 if the function failed.

Then, we use the `bind` system call to tell the OS which network interface and port the server will accept connections from. The way this works is we fill out a `struct sockaddr_in` structure, and then call `bind` with it. The `struct sockaddr_in` structure is specific to IPv4. If we were to use other protocols we would need to fill out a different type of structure (`struct sockaddr_in6` is used by IPv6 for instance).

```c
struct sockaddr_in bind_buf;
bind_buf.sin_family = AF_INET;
bind_buf.sin_port   = htons(port);
memset(&bind_buf.sin_zero, 0, sizeof(bind_buf.sin_zero));
if (inet_pton(AF_INET, addr, &bind_buf.sin_addr) != 1) {
	close(listen_fd);
	return -1;
}

if (bind(listen_fd, (struct sockaddr*) &bind_buf, sizeof(bind_buf)) < 0) {
	close(listen_fd);
	return -1;
}
```

The structure has three fields we care about: `sin_family`, `sin_port`, `sin_addr`. The `sin_family` refers to the underlying level 3 protocol we are using, which is still `AF_INET` for IPv4. The `sin_port` field contains the port we are listening on in network byte order. The byte ordering refers to how types that are larger than one byte are represented in the CPU compared to the network. Different CPU may organize the bytes that make up multi-byte types in different ways, so in order to understand each other they need to use a fixed ordering when speaking to each other. The `htons` function, which stands for "host to network short", makes sure our 16 bit value is properly ordered. The `sin_addr` field contains the IPv4 address of the local interface we want to listen on. Our server hardware may have multiple network cards, so this is how we choose one over the other. If we wanted to listen on all available interfaces, we could use the `INADDR_ANY` value, like this:

```c
bind_buf.sin_addr.s_addr = htonl(INADDR_ANY);
```

this field also needs to be in network byte order, but we use `htonl` (host to network long) for 4 byte values.

But in general we want to be able to specify the address in dotted decimal notation, such as "192.168.0.3". To perform this conversion we use the `inet_pton` function, which returns takes the dotted decimal notation string as second argument and writes the raw IP address in network byte order in the last argument. The first argument tells it which IP version to use, so AF_INET for IPv4 and AF_INET6 for IPv6.

We also have a fourth field `sin_zero`, but it's not important. I usually set it to zero just because it feel more correct this way.

And finally we call `bind` to bind the socket to the specified interface and port. Before passing the structure as second argument we need to cast its pointer to `struct sockaddr*`. The idea here is that `bind` may be used for many different protocols requiring may different configuration structures `struct sockaddr_in` for IPv4, `struct sockaddr_in6` for IPv6, `struct sockaddr_un` for UNIX sockets. For this reason `bind` a pointer to `struct sockaddr` which is an opaque type used to refer to any `struct sockaddr_*` structure. In other words it's a way to perform polymorphism.

Now that our socket is bound to an interface, we can start listening for incoming connections:
```c
if (listen(listen_fd, 32) < 0) {
	close(listen_fd);
	return -1;
}
```
This will tell the OS to start enqueueing TCP connections in the kernel's buffer associated to this socket. The second argument is called the "backlog" and is the length of that kernel queue. 

Now the server will use the `accept` system call to get connections from the kernel's queue. The way we set up things, a connection is accepted, processend, and closed. Then, the server calls accept again to process a new connection. The server will go on forever like this until we tell it to stop.

```c
for (;;) {

	int accepted_fd = accept(listen_fd, NULL, NULL);
	if (accepted_fd == -1)
		continue;

	printf("Incoming connection!\n");

	// TODO: The HTTP server logic goes here

	close(accepted_fd);
}
```

similarly to the `socket` system call, `accept` returns a descriptor to the newly accepted connection, or -1 on failure. We can read bytes from the peer by using the `recv` system call or send using the `send` system call on this descriptor. When we are done communicating, we can call `close`.

One thing to note is that the kernel relies on our server to accept incoming connections before the kernel queue is full. If we get more requests than the backlog before calling accept, the operating system will be forced to drop some connections. This is very uncommon, but can cause some head scratches when stress testing it. I know it did for me at least!

Of course this isn't even close to being a working HTTP server, but we can still spin it up and see what happens when we try to send requests to it. Let's compile it

```
$ gcc http_server.c -o http_server -Wall -Wextra -g3 -O0
```

and now we open our (still not implemented) website from our preferred browser at `http://127.0.0.1:8080/hello`.