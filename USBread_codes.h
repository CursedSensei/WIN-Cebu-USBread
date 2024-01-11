#pragma once

enum server_code {
    USBread_HOST = 0x2,
    USBread_CLIENT,
};

struct server_packet {
    unsigned char code;
    unsigned char data[4];
}