#pragma once

struct queue {

private:
	int stateKey[50];

public:
	void append(int state) {
		int i = 0;
		while (stateKey[i] != NULL) { i++; }
		stateKey[i] = state;
		i++;
		while (stateKey[i] != NULL) {
			stateKey[i] = NULL;
			i++;
		}
	}

	int get() {
		int buffer = stateKey[0];
		int i;
		for (i = 1; stateKey[i] != NULL; i++) { stateKey[i - 1] = stateKey[i]; }
		stateKey[i - 1] = NULL;
		return buffer;
	}

};

struct youthArgs {
	SOCKET mainSock;
	queue* queueList;
};

DWORD WINAPI keyThread(LPVOID args) {
	int Rbuffer = 0;
	int PGRbuffer = 0;
	int Lbuffer = 0;
	int PGLbuffer = 0;
	int i;
	while (main_exitcall) {
		i = HIBYTE(GetKeyState(VK_RIGHT));
		if (Rbuffer != i) {
			Rbuffer = i;
			if (i != 0) { ((queue*)args)->append(1); }
		}
		i = HIBYTE(GetKeyState(VK_LEFT));
		if (Lbuffer != i) {
			Lbuffer = i;
			if (i != 0) { ((queue*)args)->append(2); }
		}
		i = HIBYTE(GetKeyState(VK_NEXT));
		if (PGRbuffer != i) {
			PGRbuffer = i;
			if (i != 0) { ((queue*)args)->append(1); }
		}
		i = HIBYTE(GetKeyState(VK_PRIOR));
		if (PGLbuffer != i) {
			PGLbuffer = i;
			if (i != 0) { ((queue*)args)->append(2); }
		}
		Sleep(30);
	}
	return 0;
}

SOCKET getResolverSocket();

DWORD WINAPI youthThread(LPVOID args) {
	SOCKET MainSock = ((youthArgs *)args)->mainSock;
	queue* keyQueue = ((youthArgs *)args)->queueList;
	HeapFree(GetProcessHeap(), 0, args);

	unsigned char code;
	while (recv(MainSock, (char*)&code, 1, 0) > 0) {
		if (code == USBread_YOUTH) {
			SOCKET resolveSock = getResolverSocket();
			if (resolveSock == INVALID_SOCKET) {
				keyQueue->append(USBread_ERROR);
				continue;
			}

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
					send(resolveSock, (char*)youthFile.data, 1, 0);
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
				keyQueue->append(USBread_Empty);
				continue;
			}

			if (dataPacket.code == USBread_ERROR) {
				HeapFree(GetProcessHeap(), 0, youthFile.data);
				keyQueue->append(USBread_ERROR);
				continue;
			}

			char path[150];
			GetEnvironmentVariableA("USERPROFILE", path, 150);

			strcat_s(path, "\\Desktop\\Youth Posters\\YOUTH ");

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

			if (fp == ERROR) keyQueue->append(USBread_ERROR);
			else keyQueue->append(USBread_SUCCESS);
		}
	}

	return 0;
}

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

void getFromResolver(struct server_packet *packet) {
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


struct ipData {
private:
	char* allIp = nullptr;
	int offset = 0;

	char* getArp() {
		HANDLE pipeA = INVALID_HANDLE_VALUE;
		HANDLE pipeAwrite = INVALID_HANDLE_VALUE;

		SECURITY_ATTRIBUTES attr;
		ZeroMemory(&attr, sizeof(SECURITY_ATTRIBUTES));
		attr.nLength = sizeof(SECURITY_ATTRIBUTES);
		attr.bInheritHandle = TRUE;

		if (CreatePipe(&pipeA, &pipeAwrite, &attr, 0) == 0) { return nullptr; }
		if (!SetHandleInformation(pipeA, HANDLE_FLAG_INHERIT, 0)) { return nullptr; }

		STARTUPINFOA sInfo;
		ZeroMemory(&sInfo, sizeof(STARTUPINFOA));
		sInfo.cb = sizeof(STARTUPINFOA);
		sInfo.dwFlags = STARTF_USESTDHANDLES;
		sInfo.hStdOutput = pipeAwrite;

		PROCESS_INFORMATION pInfo;
		ZeroMemory(&pInfo, sizeof(PROCESS_INFORMATION));

		if (!CreateProcessA(NULL, (LPSTR)"arp -a", nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &sInfo, &pInfo)) { return nullptr; }

		WaitForSingleObject(pInfo.hProcess, INFINITE);
		CloseHandle(pInfo.hProcess);
		CloseHandle(pInfo.hThread);
		CloseHandle(pipeAwrite);

		char* rBuffer = (char*)malloc(32 * sizeof(char));
		if (rBuffer == nullptr) { return nullptr; }

		char rBuf[32];
		DWORD bRead, nReaded = 0;
		bool reStatus = FALSE;

		while (1) {
			reStatus = ReadFile(pipeA, rBuf, 32, &bRead, NULL);
			if (!reStatus || bRead == 0) break;
			memcpy(rBuffer + nReaded, rBuf, bRead);
			nReaded += bRead;
			if (bRead == 32) {
				rBuffer = (char*)realloc(rBuffer, nReaded + 32);
				if (rBuffer == nullptr) { return nullptr; }
			}
			else {
				memset(rBuffer + nReaded, 0, 1);
				break;
			}
		}

		CloseHandle(pipeA);

		return rBuffer;
	}

public:
	int initIp() {
		char* rBuffer = getArp();
		if (rBuffer == nullptr) return 1;
		
		int nReaded = 0;

		for (char* nBuf = rBuffer; strstr(nBuf, "dynamic") != nullptr; nReaded++) {
			nBuf = strstr(nBuf, "dynamic");
			nBuf++;
		}

		if (nReaded != 0) {
			allIp = (char*)malloc(sizeof(char) * 25 * nReaded);
			if (allIp == nullptr) return 2;

			int bRead = -1;

			for (char* nBuf = rBuffer; nReaded; nReaded--) {
				nBuf = strstr(nBuf, "dynamic");

				while (*nBuf != '\n') nBuf--;
				for (nBuf++; *nBuf == ' '; nBuf++);

				while (*nBuf != ' ') {
					bRead++;
					allIp[bRead] = *nBuf;
					nBuf++;
				}

				allIp[++bRead] = ' ';
				nBuf = strstr(nBuf, "dynamic") + 1;
			}

			allIp[bRead] = '\0';
		}

		free(rBuffer);

		return 0;
	};

	char* getIp() {
		if (allIp == nullptr || allIp[offset] == '\0') return nullptr;

		char ipnow[25] = "";
		int i;

		for (i = 0; allIp[offset + i] != ' ' && allIp[offset + i] != '\0'; i++) {
			ipnow[i] = allIp[offset + i];
		}

		if (allIp[offset + i] == '\0') offset += i; else offset += i + 1;

		return ipnow;
	}

	void dispose() {
		if (allIp != nullptr) free(allIp);
		offset = 0;
	}
};

SOCKET connSocket() {
	addrinfo sockaddr;
	PADDRINFOA hostinfo = NULL;

	ZeroMemory(&sockaddr, sizeof(sockaddr));
	sockaddr.ai_family = AF_INET;
	sockaddr.ai_socktype = SOCK_STREAM;
	sockaddr.ai_protocol = IPPROTO_TCP;

	SOCKET MainSock = INVALID_SOCKET;
	ipData ipaddr;

	short int reRun = 0;
Rerun:

	if (!ipaddr.initIp()) {
		char* ipNow = ipaddr.getIp();
		while (ipNow) {

			if (getaddrinfo(ipNow, DEFAULT_PORT, &sockaddr, &hostinfo)) {
				ipaddr.dispose();
				return INVALID_SOCKET;
			}

			for (hostinfo; hostinfo != NULL; hostinfo = hostinfo->ai_next) {

				MainSock = socket(hostinfo->ai_family, hostinfo->ai_socktype, hostinfo->ai_protocol);
				if (MainSock == INVALID_SOCKET) {
					freeaddrinfo(hostinfo);
					ipaddr.dispose();
					return INVALID_SOCKET;
				}

				if (connect(MainSock, hostinfo->ai_addr, hostinfo->ai_addrlen) == SOCKET_ERROR) {
					closesocket(MainSock);
					MainSock = INVALID_SOCKET;
					continue;
				}

				unsigned long long pass = USBread_INCOMP;

				if (send(MainSock, (char*)&pass, 1, 0) == SOCKET_ERROR) {
					closesocket(MainSock);
					MainSock = INVALID_SOCKET;
					continue;
				}

				if (recv(MainSock, (char*)&pass, 8, 0) <= 0 || pass != USBread_PASS) {
					closesocket(MainSock);
					MainSock = INVALID_SOCKET;
					continue;
				}

				pass = USBread_COMP;

				if (send(MainSock, (char*)&pass, 1, 0) == SOCKET_ERROR) {
					closesocket(MainSock);
					MainSock = INVALID_SOCKET;
					continue;
				}

				break;
			}

			if (MainSock != INVALID_SOCKET) {
				freeaddrinfo(hostinfo);
				ipaddr.dispose();
				break;
			}

			ipNow = ipaddr.getIp();
		}
	}
	else {
		freeaddrinfo(hostinfo);
		return INVALID_SOCKET;
	}

	if (MainSock == INVALID_SOCKET) {
		ipaddr.dispose();

		if (reRun < 3) {

			STARTUPINFOA sInfo;
			ZeroMemory(&sInfo, sizeof(STARTUPINFOA));
			sInfo.cb = sizeof(STARTUPINFOA);
			PROCESS_INFORMATION pInfo;
			ZeroMemory(&pInfo, sizeof(PROCESS_INFORMATION));

			if (reRun < 2) {
				if (!CreateProcessA(NULL, reRun == 0 ? (LPSTR)"ping 192.168.0.185" : (LPSTR)"ping 192.168.0.189", nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &sInfo, &pInfo)) return INVALID_SOCKET;
			}
			else {
				struct server_packet packet;

				getFromResolver(&packet);

				if (*(int*)packet.data) {
					char resolvedIp[21] = "ping ";
					memset(resolvedIp + 5, 0, 21 - 5);

					unsigned char resolvedIpLen = 5;

					for (int i = 0; i < 4; i++) {
						unsigned char reverseIpVar = resolvedIpLen;
						do {
							for (int j = resolvedIpLen++; j > reverseIpVar; j--) resolvedIp[j] = resolvedIp[j - 1];
							resolvedIp[reverseIpVar] = (packet.data[i] % 10) + '0';
							packet.data[i] = packet.data[i] / 10;
						} while (packet.data[i] != 0);

						if (i != 3) resolvedIp[resolvedIpLen++] = '.';
					}

					if (!CreateProcessA(NULL, resolvedIp, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &sInfo, &pInfo)) return INVALID_SOCKET;
				}
				else {
					return INVALID_SOCKET;
				}
			}
			
			reRun++;

			CloseHandle(pInfo.hThread);
			WaitForSingleObject(pInfo.hProcess, INFINITE);
			CloseHandle(pInfo.hProcess);

			goto Rerun;
		}

		return INVALID_SOCKET;
	}

	return MainSock;
}

SOCKET initSocket() {
	WSADATA wsData;
	if (WSAStartup(MAKEWORD(2, 2), &wsData)) return INVALID_SOCKET;

	SOCKET MainSock = connSocket();
	if (MainSock == INVALID_SOCKET) WSACleanup();

	return MainSock;
}