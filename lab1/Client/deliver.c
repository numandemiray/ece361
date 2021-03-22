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
	- https://stackoverflow.com/questions/6002528/c-serialization-techniques0
	- https://stackoverflow.com/questions/7887003/c-sscanf-not-working
    -https://stackoverflow.com/questions/13547721/udp-socket-set-timeout

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
#include <time.h>
#include <math.h>

#define MAXBUFLEN 100

struct packet {
    int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char filename[100];
    char filedata[1000];

};

void serialize(struct packet * structPacket, char * data) {

    // add first four parts using s printf
    sprintf(data, "%d:%d:%d:%s:", structPacket -> total_frag, structPacket -> frag_no, structPacket -> size, structPacket -> filename);

    int count = 0;
    int i = 0;
    while (count < 4) {
        if (data[i] == ':') {
            count++;
        }
        i++;
    }
    int startingIndex = i;

    // add data using memcpy
    memcpy(data + startingIndex, structPacket -> filedata, structPacket -> size + 1);

}

// deliver < server address > < server port number>
int main(int argc, char * argv[]) {
    int sockfd;
    struct addrinfo hints, * servinfo, * p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char buf[MAXBUFLEN];

    if (argc != 3) {
        fprintf(stderr, "<server address > < server port number>\n");
        exit(1);
    }

    memset( & hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(argv[1], argv[2], & hints, & servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // make a socket
    for (p = servinfo; p != NULL; p = p -> ai_next) {
        if ((sockfd = socket(p -> ai_family, p -> ai_socktype,
                p -> ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }
    freeaddrinfo(servinfo);

    // ask user to enter file name
    char input[MAXBUFLEN];

    printf("Enter file name with format: ftp < file name>\n");
    fgets(input, MAXBUFLEN, stdin);

    input[strlen(input) - 1] = '\0';

    //check if correct format
    char check_format[4];
    strncpy(check_format, input, 4);
    check_format[4] = '\0';

    if (strcmp(check_format, "ftp ") != 0) {
        fprintf(stderr, "Wrong format: ftp <file name>\n");
        return 1;
    }
    // cut out ftp 
    char * fileName = input + 4;

    // check if file exists
    FILE * file;
    file = fopen(fileName, "r");
    if (file == NULL) {
        fprintf(stderr, "File does not exist in current directory\n");
        return 1;
    } else {
        fclose(file);

        // start timer 
        clock_t start = clock();

        //send
        if ((numbytes = sendto(sockfd, "ftp", strlen("ftp"), 0,
                p -> ai_addr, p -> ai_addrlen)) == -1) {
            perror("talker: sendto");
            exit(1);
        }

        printf("Client is waiting to receive from Server\n");
        //receive

        addr_len = sizeof their_addr;
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                (struct sockaddr * ) & their_addr, & addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        // end timer
        clock_t end = clock();
        float seconds = (float)(end - start) / CLOCKS_PER_SEC;

        printf("Round-trip Time: %f\n", seconds);
        buf[numbytes] = '\0';

        // if the message is yes, print out a file transfer can start
        if (strcmp(buf, "yes") == 0) {
            printf("A file transfer can start.\n");
        }

        // --- SECTION 3 file sending part ---

        // read file
        FILE * file;
        file = fopen(fileName, "rb");
        if (file == NULL) {
            fprintf(stderr, "File does not exist in current directory\n");
            return 1;
        }

        // find number of packets required
        unsigned int fileSize;
        fseek(file, 0L, SEEK_END);
        fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);
        unsigned int total_frag = ((fileSize - 1) / 1000) + 1;

        char sendThis[2000];
        struct packet temp;
        // CALCULATE RTT FOR EACH PACKET
        // for each packet, send 
        float sampleRTT = 0;
        float estimatedRTT = 0;
        float devRTT = 0;
        float timeoutInterval = 1.0;
        int timeoutNumber = 0;

        for (int i = 0; i < total_frag; i++) {

            // make each struct
            memset(sendThis, 0, sizeof(char) * 2000);
            memset(temp.filedata, 0, sizeof(char) * 1000);

            fread(temp.filedata, sizeof(char), 1000, file);

            temp.total_frag = total_frag;
            temp.frag_no = i + 1;
            strcpy(temp.filename, fileName);
            // if last packet, then find remaining size
            if ((i + 1) == total_frag) {
                fseek(file, 0, SEEK_END);
                temp.size = (ftell(file) - 1) % 1000 + 1;
            } else {
                temp.size = 1000;
            }

            serialize( & temp, sendThis);

            clock_t startRTTtimer = clock();

            //send
            if ((numbytes = sendto(sockfd, sendThis, 1100, 0,
                    p -> ai_addr, p -> ai_addrlen)) == -1) {
                printf("Error sending packet %d\n", temp.frag_no);
                perror("talker: sendto");
                exit(1);
            }
            
            printf("Sending packet %d   Timeout: %f\n",i+1, timeoutInterval);
            //receive ack
            memset(buf, 0, sizeof(buf));

            // wait for receive
            struct timeval timeout;
            timeout.tv_sec = timeoutInterval * 10000;
            if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char * ) & timeout, sizeof(struct timeval)) < 0) {
                perror("Error");
            }
            if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                    (struct sockaddr * ) & their_addr, & addr_len)) < 0) {
                printf("Timeout reached. Resending packet %d\n", i + 1);
                i--;
                timeoutNumber++;
                if (timeoutNumber > 5) {
                    printf("Maximum attempts of resend reached. Closing Client\n");
                    exit(1);
                }
            }

            clock_t endRTTtimer = clock();
            sampleRTT = (float)(endRTTtimer - startRTTtimer) / CLOCKS_PER_SEC;
            estimatedRTT = 0.875 * estimatedRTT + sampleRTT;
            devRTT = 0.75 * devRTT * 0.25 * fabs(sampleRTT - estimatedRTT);
            timeoutInterval = estimatedRTT + (4 * devRTT);
            //printf("second timeout interval: %f\n",timeoutInterval);

        }
        fclose(file);
        close(sockfd);
        return 0;
    }
}