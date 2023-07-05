#pragma once

int main_client() {
	WSADATA wsData;
	if (WSAStartup(MAKEWORD(2,2), &wsData)) {
		return 1;
	}

	addrinfo sockaddr;
	PADDRINFOA hostinfo = NULL;

	ZeroMemory(&sockaddr, sizeof(sockaddr));
	sockaddr.ai_family = AF_INET;
	sockaddr.ai_socktype = SOCK_STREAM;
	sockaddr.ai_protocol = IPPROTO_TCP;

	SOCKET MainSock = INVALID_SOCKET;

	FILE* outpipe = _popen("arp -a", "r");
	if (outpipe != NULL) {
		char buffered[128];
		char ipNow[25];
		while (fgets(buffered, sizeof(buffered), outpipe) != NULL) {
			if (strstr(buffered, "dynamic")) {
				strcpy_s(ipNow, "");
				int i = 0;
				while (buffered[i] == ' ') { i++; }
				for (i; buffered[i] != ' '; i++) {
					strncat_s(ipNow, &buffered[i], 1);
				}
			}
			else { continue; }

			if (getaddrinfo(ipNow, DEFAULT_PORT, &sockaddr, &hostinfo)) {
				WSACleanup();
				return 2;
			}

			for (hostinfo; hostinfo != NULL; hostinfo = hostinfo->ai_next) {

				MainSock = socket(hostinfo->ai_family, hostinfo->ai_socktype, hostinfo->ai_protocol);
				if (MainSock == INVALID_SOCKET) {
					freeaddrinfo(hostinfo);
					WSACleanup();
					return 31;
				}

				if (connect(MainSock, hostinfo->ai_addr, hostinfo->ai_addrlen) == SOCKET_ERROR) {
					closesocket(MainSock);
					MainSock = INVALID_SOCKET;
					continue;
				}
				break;
			}

			if (MainSock != INVALID_SOCKET) {
				break;
			}

		}
	}
	else {
		freeaddrinfo(hostinfo);
		WSACleanup();
		return 32;
	}
	_pclose(outpipe);

	if (MainSock == INVALID_SOCKET) {
		WSACleanup();
		return 33;
	}

	freeaddrinfo(hostinfo);

	if (shutdown(MainSock, SD_RECEIVE) == SOCKET_ERROR) {
		closesocket(MainSock);
		WSACleanup();
		return 3;
	}

	queue *keyQueue = (queue *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(queue));
	HANDLE keyhandle = CreateThread(NULL, 0, keyThread, (void*)keyQueue, 0, NULL);
	if (!keyhandle) {
		closesocket(MainSock);
		WSACleanup();
		return 3;
	}
	CloseHandle(keyhandle);

	modtray();

	int queueKey;
	char keyName[6];
	int Nullsend = 0;

	while (main_exitcall) {
		queueKey = keyQueue->get();
		if (queueKey != 0 || Nullsend > 100) {
			if (queueKey == 1) { strcpy_s(keyName,"Right"); }
			else if (queueKey == 2) { strcpy_s(keyName,"Left "); }
			else { strcpy_s(keyName, "NONE "); }

			if (send(MainSock, keyName, strlen(keyName), 0) == SOCKET_ERROR) {
				main_exitcall = 0;
				closesocket(MainSock);
				WSACleanup();
				HeapFree(GetProcessHeap(), NULL, keyQueue);
				return 4;
			}

			Nullsend = 0;
		}
		else {
			Nullsend++;
			Sleep(30);
		}
	}

	closesocket(MainSock);
	WSACleanup();
	HeapFree(GetProcessHeap(), NULL, keyQueue);
	return 0;
}

DWORD WINAPI client_thread(LPVOID args) {
	int status = 0;
	HWND mainwin = *(HWND*)args;
	HeapFree(GetProcessHeap(), NULL, args);

	status = main_client();

	PostMessage(mainwin, WM_CLOSE, 0, 0);
	return status;
}