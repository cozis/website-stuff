# Building web apps from scratch - Parsing Requests - Part 6

## Receiving Requests

In the last post we set up a TCP server capable of accepting connections and then closing them, but was doing nothing with them. In this post we will read a request and parse it.

The inner portion of our accept loop looked like this:

```c
int accepted_fd = accept(listen_fd, NULL, NULL);
if (accepted_fd < 0)
	continue;

printf("Incoming connection!\n");

// TODO: The HTTP server logic goes here

close(accepted_fd);
```

Now lets create a buffer for our incoming request and call the `recv` system call to write into it:

```c
int accepted_fd = accept(listen_fd, NULL, NULL);
if (accepted_fd < 0)
	continue;

printf("Incoming connection!\n");

char input_buffer[4000];
int input_size = recv(accepted_fd, input_buffer, sizeof(input_buffer), 0);
if (input_size > 0 && input_size < sizeof(input_buffer)) {

	printf("=== REQUEST ===\n");
	printf("%.*s\n", input_size, input_buffer);
	printf("===============\n");

	// TODO: The HTTP server logic goes here

}
close(accepted_fd);
```

The call to `recv` will return -1 if an I/O error occurred, 0 if the peer disconnected, or the number of bytes written to the buffer if the read was successfull. If we get a non-positive return value we must skip the processing step.

We also need to account for the limited size of the buffer. It's possible for the request to not fit in the fixed buffer, so we must consider this scenario as an error. The way we detect this is by checking that at least one byte of the buffer wasn't written to. If the buffer gets filled up by recv, we assume more bytes needed to be written. In a bit we will change this code to use dynamic buffers, which will solve this problem entirely.

One other simplification we are doing for now is assuming the entire request will be received in one `recv` call. This is not necessarily the case. Since TCP is not a message-oriented protocol there is no reason a single `recv` will write the entire request. Since we are not stress testing our server yet we will likely get the expected behavior anyways, but it's possible for partial request to be read. Our implementation will handle these as malformed request due to a client error, even though it's our fault.

If we run the above code and open our server's address in a browser, we should get a log of the received HTTP requests.

## Adding a Dynamic Buffer and Addressing Partial Reads

First, let's fix the buffer limit issue.

The idea here is that we allocate a dynamic buffer and `recv` into it. If the request was received completely, we are done. If it wasn't, we grow the buffer and `recv` again. We can also use this logic to address partial ready by calling `recv` without growing the array if the last call did not use all free space.

But how do we know whether the buffer contains a complete HTTP request or not? This is more tricky.

We know that the head of the request is always terminated by the \r\n\r\n sequence. If we can't find it in our received bytes, the head of the request wasn't received yet. If we do find it, then the head was received but the body may or may not. We need to parse the "Content-Length" header to know whether the body was received too.

Let's start with the dynamic buffer:

```c
int accepted_fd = accept(listen_fd, NULL, NULL);
if (accepted_fd < 0)
	continue;

printf("Incoming connection!\n");

#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))

char *input_buffer = NULL;
int   input_size = 0;
int   input_capacity = 0;
int   error = 0;
for (;;) {

	int min_recv = 1024;
	if (input_capacity - input_size < min_recv) {

		input_capacity = MAX(min_recv, 2*input_capacity);
		input_buffer = realloc(input_buffer, input_capacity);
		if (input_buffer == NULL) {
			error = 1;
			break;
		}
	}

	int num = recv(accepted_fd, input_buffer + input_size, input_capacity - input_size);
	if (num <= 0) {
		error = 0;
		break;
	}
	input_size += num;

	num = parse_request(input_buffer, input_size);
	if (num < 0) {
		error = 1;
		break;
	}

	if (num > 0)
		break;
}

free(input_buffer)
close(accepted_fd);
```

Now the call to `recv` is performed multiple times. Before each call we make sure there is some free space in the input buffer. After the call, we try to parse the request. If the request is available, a positive number is returned so we can exit the loop. If the request is malformed, a negative value is returned. If the return value is zero, we need to receive more bytes. If for any reason we exit the loop because of some problem, se set the error flag.

