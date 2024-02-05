#pragma once

#define RESOLVER_IPNAME "tunnel.area51hirishima.site"
#define RESOLVER_PORT "30003"
#define RESOLVER_LISTENPORT 30003

enum server_code {
    USBread_HOST = 0x2,
    USBread_CLIENT,
};

struct server_packet {
    unsigned char code;
    unsigned char data[4];
}