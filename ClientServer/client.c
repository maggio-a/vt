#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char *argv[])
{
	int sck;
	struct sockaddr_in address;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s msg\n", argv[0]);
		return -1;
	}

	sck = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	address.sin_family = AF_INET;
	address.sin_port = 12345;
	//inet_pton(AF_INET, "127.0.0.1", &address.sin_addr.s_addr);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	printf("%s %zd\n", argv[1], strlen(argv[1]));

	connect(sck, (struct sockaddr *) &address, sizeof(address));
	size_t n = strlen(argv[1]);
	ssize_t s1 = send(sck, argv[1], n+1, 0);
	memset(argv[1], 0x00, strlen(argv[1]));
	assert(argv[1][0] == '\0');
	ssize_t s2 = recv(sck, argv[1], n+1, 0);
	printf("%zd %zd\n", s1, s2);
	fprintf(stdout, "Server replied \"%s\"\n", argv[1]);
	close(sck);

	return 0;
}