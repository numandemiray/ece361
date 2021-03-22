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

#define MAXBUFLEN 1100

struct packet {
    int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char filename[100];
    char filedata[1000];

};

void deserialize(char * data, struct packet * msgPacket) {
    // get the first 3 things into the struct
    int index = 0;
    char total_frag_str[10];
    char frag_no_str[10];
    char size_str[100];
    char filename_str[100];


    sscanf(data, "%[^:]:%[^:]:%[^:]:%[^:]:", total_frag_str, frag_no_str, size_str, filename_str);
    msgPacket -> total_frag = atoi(total_frag_str);
    msgPacket -> frag_no = atoi(frag_no_str);
    msgPacket -> size = atoi(size_str);
    strcpy(msgPacket -> filename, filename_str);

    int count = 0;
    int i = 0;
    while (count < 4) {
        if (data[i] == ':') {
            count++;
        }
        i++;
    }
    int startingIndex = i;
    memcpy( & (msgPacket -> filedata), data + startingIndex, msgPacket -> size);

}

// server < UDP listen port>
int main(int argc, char * argv[]) {
    int sockfd;
    struct addrinfo hints, * servinfo, * p;
    int rv;
    int numbytes;
    char buf[MAXBUFLEN];
    struct sockaddr_storage their_addr;
    socklen_t addr_len;

    if (argc != 2) {
        fprintf(stderr, "usage:<UDP listen port>\n");
        exit(1);
    }

    memset( & hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    // get address info
    if ((rv = getaddrinfo(NULL, argv[1], & hints, & servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // create socket
    for (p = servinfo; p != NULL; p = p -> ai_next) {
        if ((sockfd = socket(p -> ai_family, p -> ai_socktype,
                p -> ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }
        // bind to socket
        if (bind(sockfd, p -> ai_addr, p -> ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("Server is waiting to receive from client\n");

    // receive 
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
            (struct sockaddr * ) & their_addr, & addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    buf[numbytes] = '\0';

    // send
    if (strcmp(buf, "ftp") == 0) {
        if ((numbytes = sendto(sockfd, "yes", strlen("yes"), 0,
                (struct sockaddr * ) & their_addr, sizeof(struct sockaddr_storage)) == -1)) {
            perror("talker: sendto");
            exit(1);
        }
    } else {

        if ((numbytes = sendto(sockfd, "no", strlen("no"), 0,
                (struct sockaddr * ) & their_addr, sizeof(struct sockaddr_storage)) == -1)) {
            perror("talker: sendto");

            exit(1);
        }
    }

    // ---SECTION 3 file receiving part---

    struct packet packet;
    FILE * file;

    int notFinished = 0;
    while (notFinished == 0) {
        
        // receive
        if ((numbytes = recvfrom(sockfd, buf, 1100, 0,
                (struct sockaddr * ) & their_addr, & addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        deserialize(buf, & packet);

        // write to file
        unsigned int error;
        // if first file, then create a new file
        if (packet.frag_no == 1) {
            
            file = fopen(packet.filename, "wb");
            error = fwrite(packet.filedata, packet.size, 1, file);
            
            if (error != 1) {
                fprintf(stderr, "could not write");
                return 1;
            }
        } else {
            error = fwrite(packet.filedata, packet.size, 1, file);
            if (error != 1) {
                fprintf(stderr, "could not write");
                return 1;
            }

        }
        if(packet.frag_no % 10 == 0){
            sleep(1);
        }
        //send ack
        if ((numbytes = sendto(sockfd, "ack", strlen("ack"), 0,
                (struct sockaddr * ) & their_addr, sizeof(struct sockaddr_storage)) == -1)) {
            perror("talker: sendto");

            exit(1);
        }
        // if last packet then stop loop
        if (packet.frag_no == packet.total_frag) {
            printf("The %s file is transfered\n", packet.filename);
            notFinished = 1;
        }

    }

    // close the socket  
    fclose(file);
    close(sockfd);
    return 0;
}
// check packet number, 