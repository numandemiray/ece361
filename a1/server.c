/*
1. Open a UDP socket and listen at the specified port number

2. Receive a message from the client
    a. if the message is “ftp”, reply with a message “yes” to the client.

    b. else, reply with a message “no” to the client.

References
    - Beej's Guide to Network Programming
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define MAXBUFLEN 100
// server < UDP listen port>

int main(int argc, char *argv[])
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	char buf[MAXBUFLEN];
	struct sockaddr_storage their_addr;
	socklen_t addr_len;

	if (argc != 2)
	{
		fprintf(stderr, "usage:<UDP listen port>\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	// get address info
	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// create socket
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1)
		{
			perror("listener: socket");
			continue;
		}
		// bind to socket
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("listener: bind");
			continue;
		}
		break;
	}
	if (p == NULL)
	{
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	// receive 

	addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
			(struct sockaddr *) &their_addr, &addr_len)) == -1)
	{
		perror("recvfrom");
		exit(1);
	}

	buf[numbytes] = '\0';

	// send

	if (strcmp(buf, "ftp") == 0)
	{
		if ((numbytes = sendto(sockfd, "yes", strlen("yes"), 0,
				(struct sockaddr *) &their_addr, sizeof(struct sockaddr_storage)) == -1))
		{
			perror("talker: sendto");
			exit(1);
		}
	}
	else
	{

		if ((numbytes = sendto(sockfd, "no", strlen("no"), 0,
				(struct sockaddr *) &their_addr, sizeof(struct sockaddr_storage)) == -1))
		{
			perror("talker: sendto");

			exit(1);
		}
	}

	// close the socket    

	close(sockfd);
	return 0;
}
