#include <linux/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define SOCKET int

#include "../USBread_codes.h"

volatile unsigned char keepRunning = 1;
SOCKET listenerSock;

void cleanup(int dumm) {
    close(listenerSock);
    keepRunning = 0;
}

int isInvalid(SOCKET socket) {
    return socket < 0 ? 1 : 0;
}

int main() {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    listenerSock = socket(AF_INET, SOCK_STREAM, 0);
    if (isInvalid(listenerSock)) {
        return 1;
    }

    struct sockaddr_in bindaddr;
    bindaddr.sin_addr.s_addr = INADDR_ANY;
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_port = htons(RESOLVER_LISTENPORT);

    if (bind(listenerSock, (struct sockaddr *)&bindaddr, sizeof(struct sockaddr_in)) < 0) {
        close(listenerSock);
        return 2;
    }

    if (listen(listenerSock, 5)) {
        close(listenerSock);
        return 3;
    }

    unsigned char ip[4];
    struct server_packet packet;
    memset(ip, 0, 4);

    while (keepRunning) {
        memset(&packet, 0, sizeof(struct server_packet));

        SOCKET clientSock = accept(listenerSock, NULL, NULL);
        if (isInvalid(clientSock)) {
            close(clientSock);
            continue;
        }

        for (int retries = 20; retries; retries--) {
            if (recv(clientSock, (void*)&packet, sizeof(struct server_packet), MSG_DONTWAIT) == -1) {
                if (errno == EWOULDBLOCK) {
                    usleep(500000);
                    continue;
                }
            }

            break;
        }

        switch (packet.code) {
            case USBread_HOST:
                memcpy(&ip, packet.data, 4);
                break;
            case USBread_CLIENT:
                send(clientSock, (void *)&ip, 4, MSG_NOSIGNAL);
                break;
            case USBread_YOUTH:
                {
                    FILE *fp = fopen("/home/aoi/Desktop/ElquencePage/WinPage/YOUTH.png", "rb");
                    if (fp == NULL) {
                        unsigned char nullCode[2];
                        nullCode[0] = USBread_NODATA;
                        send(clientSock, (void *)&nullCode, 2, MSG_NOSIGNAL);
                    } else {
                        fseek(fp, 1, SEEK_END);
                        unsigned long long fileLen = ftell(fp);

                        rewind(fp);

                        struct data_packet filePacket;
                        filePacket.code = USBread_INCOMP;
                        while (fileLen >= sizeof(struct data_packet)) {
                            recv(clientSock, (void *)filePacket.data, 1, 0);
                            fread(filePacket.data, 1, sizeof(struct data_packet) - 1, fp);

                            send(clientSock, (void *)&filePacket, sizeof(struct data_packet), MSG_NOSIGNAL);

                            fileLen -= 1499;
                        }
                        
                        filePacket.code = USBread_COMP;
                        fread(filePacket.data, 1, fileLen, fp);
                        send(clientSock, (void *)&filePacket, fileLen, MSG_NOSIGNAL);

                        fclose(fp);
                    }
                }
                break;
        }

        close(clientSock);
    }
    return 0;
}
