#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

const int MAXLENGTH = 50;

int createServerChannel()
{
	struct sockaddr_in server_addr;
	int socket_fd;

	socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket_fd < 0) {
		perror("socket");
		return -1;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = 12345;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr))) {
		perror("bind");
		return -1;
	}

	return socket_fd;
}

int main(int argc, char *argv[])
{
	int sc;
	struct sockaddr_in incoming_addr;
	socklen_t incoming_addrsize;

	char buffer[MAXLENGTH];
	memset(&buffer, 0x00, MAXLENGTH);

	if ((sc = createServerChannel()) < 0) {
		perror("createServerChannel");
		return -1;
	}

	if (listen(sc, 5)) {
		perror("listen");
		return -1;
	}

	for (;;) {
		incoming_addrsize = sizeof(incoming_addr);
		int client = accept(sc, (struct sockaddr *) &incoming_addr, &incoming_addrsize);

		if (client == -1) {
			perror("accept");
			return -1;
		} else {
			ssize_t n = recv(client, buffer, MAXLENGTH-1, 0);
			printf("%zu\n", n);

			if (n < 0) {
				perror("recv");
				close(client);
			} else {
				buffer[n] = '\0';
				printf("Received %s\n", buffer);
				n = send(client, buffer, strlen(buffer)+1, 0);

				if (n < 0) {
					perror("send");
				}

				close(client);
			}
		}
	}

	return 0;
}