#pragma once

#define USBread_PASS 0x4d78ab239938e35f

#define RESOLVER_IPNAME "tunnel.area51hirishima.site"
#define RESOLVER_PORT "30003"
#define RESOLVER_LISTENPORT 30003

enum key_codes {
    USBread_LEFT = 1,
    USBread_RIGHT,

    USBread_SUCCESS,
    USBread_ERROR,
    USBread_Empty
};

enum server_code {
    USBread_HOST = 0x2,
    USBread_CLIENT,
    
    USBread_YOUTH,

    USBread_INCOMP,
    USBread_COMP,
    USBread_NODATA
};

struct server_packet {
    unsigned char code;
    unsigned char data[4];
};

struct data_packet {
    unsigned char code;

    unsigned char data[1499];
};

struct data_File {
    unsigned long long len;
    unsigned char *data;
};