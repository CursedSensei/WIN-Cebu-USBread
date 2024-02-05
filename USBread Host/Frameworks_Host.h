#pragma once
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <stdio.h>
#include <ShlObj_core.h>
#include <math.h>
#include "resource_host.h"

#pragma comment (lib, "Ws2_32.lib")

#define TRAY_CALLBACK (WM_USER + 0x100)
#define DEFAULT_PORT "28771"

#include "../USBread_codes.h"