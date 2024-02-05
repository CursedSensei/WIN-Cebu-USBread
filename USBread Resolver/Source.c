#include <linux/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SOCKET int

#include "../USBread_codes.h"

int isInvalid(SOCKET socket) {
    return socket < 0 ? 1 : 0;
}

int main() {
    SOCKET listenerSock = socket(AF_INET, SOCK_STREAM, 0);
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

    while (1) {
        SOCKET clientSock = connect(listenerSock, NULL, NULL);
        if (isInvalid(clientSock)) {
            close(clientSock);
            continue;
        }

        struct server_packet packet;

        if (recv(clientSock, (void *)&packet, sizeof(struct server_packet), 0) == -1) {
            close(clientSock);
            continue;
        }

        switch (packet.code) {
            case USBread_HOST:
                memcpy(&ip, packet.data, 4);
                break;
            case USBread_CLIENT:
                send(clientSock, (void *)&ip, 4, 0);
                break;
        }

        close(clientSock);
    }

    close(listenerSock);
    return 0;
}