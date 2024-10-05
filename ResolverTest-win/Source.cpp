#include "Frameworks.h"

SOCKET getResolverSocket() {
	addrinfo sockaddr;
	PADDRINFOA hostinfo = NULL;

	ZeroMemory(&sockaddr, sizeof(sockaddr));
	sockaddr.ai_family = AF_INET;
	sockaddr.ai_socktype = SOCK_STREAM;
	sockaddr.ai_protocol = IPPROTO_TCP;

	SOCKET MainSock = INVALID_SOCKET;

	if (getaddrinfo(RESOLVER_IPNAME, RESOLVER_PORT, &sockaddr, &hostinfo)) {
		return INVALID_SOCKET;
	}

	for (hostinfo; hostinfo != NULL; hostinfo = hostinfo->ai_next) {

		MainSock = socket(hostinfo->ai_family, hostinfo->ai_socktype, hostinfo->ai_protocol);
		if (MainSock == INVALID_SOCKET) {
			freeaddrinfo(hostinfo);
			return INVALID_SOCKET;
		}

		if (connect(MainSock, hostinfo->ai_addr, hostinfo->ai_addrlen) == SOCKET_ERROR) {
			closesocket(MainSock);
			MainSock = INVALID_SOCKET;
			continue;
		}
		break;
	}

	if (MainSock == INVALID_SOCKET) return INVALID_SOCKET;

	else freeaddrinfo(hostinfo);

	return MainSock;
}

void getFromResolver(struct server_packet* packet) {
	memset(packet->data, 0, 4);
	SOCKET resolver_server = getResolverSocket();

	if (resolver_server == INVALID_SOCKET) {
		return;
	}

	packet->code = USBread_CLIENT;

	if (send(resolver_server, (char*)packet, sizeof(struct server_packet), 0) == SOCKET_ERROR) {
		closesocket(resolver_server);
		return;
	}

	recv(resolver_server, (char*)packet->data, 4, 0);

	closesocket(resolver_server);
}

int main() {
	WSADATA wsData;
	if (WSAStartup(MAKEWORD(2, 2), &wsData)) return 1;

	int input = 1;

	while (input != 0) {
		printf("Get IP = 1\nGet Youth Poster = 2\nTest Resolver Timeout = 3\nExit = 0\n\nOp: ");

		scanf_s("%d", &input);

		switch (input) {
		case 1:
			{
				server_packet packet;
				ZeroMemory(&packet, sizeof(struct server_packet));

				getFromResolver(&packet);

				if (packet.code == 0) {
					printf("\nCannot connect to Resolver Server\n\n");
					break;
				}

				printf("\nIP: %u.%u.%u.%u\n\n", packet.data[0], packet.data[1], packet.data[2], packet.data[3]);
			}
			break;

		case 2:
			{
				SOCKET resolveSock = getResolverSocket();
				if (resolveSock == INVALID_SOCKET) {
					printf("\nCannot connect to Resolver Server\n\n");
					break;
				}

				char code = USBread_YOUTH;

				send(resolveSock, (char*)&code, 1, 0);

				struct data_File youthFile;
				youthFile.len = 0;
				youthFile.data = (unsigned char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0);

				struct data_packet dataPacket;
				int bytesReceived;
				do {
					unsigned long long curLen = youthFile.len + 112425;

					youthFile.data = (unsigned char*)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, youthFile.data, curLen);

					do {
						bytesReceived = recv(resolveSock, (char*)&dataPacket, sizeof(struct data_packet), 0);
						if (bytesReceived <= 0) {
							dataPacket.code = USBread_ERROR;
							break;
						}

						memcpy(youthFile.data + youthFile.len, dataPacket.data, (size_t)bytesReceived - 1);

						youthFile.len += (unsigned long long)bytesReceived - 1;
					} while (youthFile.len < curLen && dataPacket.code == USBread_INCOMP);
				} while (dataPacket.code == USBread_INCOMP);

				closesocket(resolveSock);

				if (dataPacket.code == USBread_NODATA) {
					HeapFree(GetProcessHeap(), 0, youthFile.data);
					printf("\nNo Youth File\n\n");
					break;
				}

				if (dataPacket.code == USBread_ERROR) {
					HeapFree(GetProcessHeap(), 0, youthFile.data);
					printf("\nFailed on Downloading\n\n");
					break;
				}

				char path[150] = "YOUTH ";

				char date_fileSuffix[20];

				GetDateFormatA(LOCALE_SYSTEM_DEFAULT, NULL, NULL, "yyyy'-'MM'-'dd", date_fileSuffix, 11);

				strcat_s(path, date_fileSuffix);

				unsigned short pathLen = strlen(path);
				unsigned short file_num = 0;
				strcpy_s(date_fileSuffix, ".png");
				ZeroMemory(date_fileSuffix + 5, 6);
				HANDLE fp;
				do {
					strcat_s(path, date_fileSuffix);

					fp = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
					if (fp == INVALID_HANDLE_VALUE) {
						switch (GetLastError()) {
						case ERROR_FILE_EXISTS:
						{
							snprintf(date_fileSuffix, 20, " (%u).png", ++file_num);
							ZeroMemory(path + pathLen, strlen(date_fileSuffix));
						}
						break;
						case ERROR_PATH_NOT_FOUND:
						{
							char directoryPath[150];
							ZeroMemory(directoryPath, 150);
							memcpy(directoryPath, path, pathLen - 17);
							if (!CreateDirectoryA(directoryPath, NULL)) {
								fp = ERROR;
							}
							else {
								ZeroMemory(path + pathLen, strlen(date_fileSuffix));
							}
						}
						break;
						default:
						{
							fp = ERROR;
						}
						break;
						}
					}
					else {
						DWORD written = 0;

						WriteFile(fp, youthFile.data, youthFile.len, &written, NULL);

						CloseHandle(fp);
					}
				} while (fp == INVALID_HANDLE_VALUE);

				HeapFree(GetProcessHeap(), 0, youthFile.data);

				if (fp == ERROR) {
					printf("\nDownload fail\n\n");
				}

				printf("\nDownload success\n\n");
			}
			break;

		case 3:
			{
				SOCKET resolveSock = getResolverSocket();
				if (resolveSock == INVALID_SOCKET) {
					printf("\nCannot connect to Resolver Server\n\n");
					break;
				}

				recv(resolveSock, nullptr, 0, 0);

				closesocket(resolveSock);

				printf("Resolver Socket closed\n\n");
			}
			break;
		}
	}



	WSACleanup();

	return 0;
}