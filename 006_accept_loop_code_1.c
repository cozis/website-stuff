#include <stdio.h> // printf

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

	for (;;) {

		SOCKET accepted_fd = accept(listen_fd, NULL, NULL);
		if (accepted_fd == INVALID_SOCKET)
			continue;

		char input[128];
		int input_len = recv(accepted_fd, input, sizeof(input), 0);
		if (input_len < 0) {
			CLOSE_SOCKET(accepted_fd);
			continue;
		}

		printf("Received message is:\n%.*s\n\n", input_len, input);

		send(accepted_fd, input, input_len, 0);

		CLOSE_SOCKET(accepted_fd);
	}

#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}