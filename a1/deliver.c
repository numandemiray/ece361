/*
After executing the server, the client should:
1. Ask the user to input a message as follows:
   ftp < file name>

2. Check the existence of the file:
    a. if exist, send a message “ftp” to the server
    b. else, exit

3. Receive a message from the server:
    a. if the message is “yes”, print out “A file transfer can start.”
    b. else, exit

References
    - Beej's Guide to Network Programming
    - https://www.mikedane.com/programming-languages/c/getting-input-from-users/
    - https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c

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
// deliver < server address > < server port number>

int main(int argc, char *argv[])
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	socklen_t addr_len;
	char buf[MAXBUFLEN];

	if (argc != 3)
	{
		fprintf(stderr, "<server address > < server port number>\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// make a socket
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1)
		{
			perror("talker: socket");
			continue;
		}
		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}
	freeaddrinfo(servinfo);

	// 1. ask user to enter file name
	char input[MAXBUFLEN];
	printf("Enter file name with format: ftp < file name>\n");
	fgets(input, MAXBUFLEN, stdin);

	input[strlen(input) - 1] = '\0';
	// cut out ftp 
	char *fileName = input + 4;

	// 2. if file exists send, o/w exit
	FILE * file;
	if ((file = fopen(fileName, "r")))
	{
		fclose(file);
		//send
		if ((numbytes = sendto(sockfd, "ftp", strlen("ftp"), 0,
				p->ai_addr, p->ai_addrlen)) == -1)
		{
			perror("talker: sendto");
			exit(1);
		}

		//receive

		addr_len = sizeof their_addr;
		if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
				(struct sockaddr *) &their_addr, &addr_len)) == -1)
		{
			perror("recvfrom");
			exit(1);
		}

		buf[numbytes] = '\0';

		// 3. if the message is yes, print out a file transfer can start
		if (strcmp(buf, "yes") == 0)
		{
			printf("A file transfer can start.\n");
		}

		close(sockfd);
	}

	return 0;

}
