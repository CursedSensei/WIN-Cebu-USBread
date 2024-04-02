#pragma once

int isCheckEnabled() {
	if (__argc > 1 && strlen(__argv[1]) == 10) {
		char Argu[] = "--no-check";
		for (int i = 0; i < 10; i++) {
			if (__argv[1][i] != Argu[i]) {
				return 1;
			}
			return 0;
		}
	}
	return 1;
}

int main_client() {
    SOCKET MainSock;
    do {
	    MainSock = initSocket();
	    if (isCheckEnabled() && MainSock == INVALID_SOCKET) {
            time_t nTime = time(NULL);
			struct tm aTime = { };
			localtime_s(&aTime, &nTime);

            if (aTime.tm_wday == 0 && aTime.tm_hour < 12) {
                Sleep(2000);
            }
            else return 1;
        }
	} while (MainSock == INVALID_SOCKET);

	queue* keyQueue = (queue*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(queue));
	youthArgs* youthparam = (youthArgs*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(youthArgs));
	youthparam->mainSock = MainSock;
	youthparam->queueList = keyQueue;

	HANDLE youthHandle = CreateThread(NULL, 0, youthThread, (void*)youthparam, 0, NULL);
	if (!youthHandle) {
		closesocket(MainSock);
		HeapFree(GetProcessHeap(), NULL, youthparam);
		HeapFree(GetProcessHeap(), NULL, keyQueue);
		WSACleanup();
		return 3;
	}
	CloseHandle(youthHandle);

	HANDLE keyhandle = CreateThread(NULL, 0, keyThread, (void*)keyQueue, 0, NULL);
	if (!keyhandle) {
		closesocket(MainSock);
		WSACleanup();
		return 3;
	}
	CloseHandle(keyhandle);

	modtray();

	int queueKey;
	char keyName;
	int Nullsend = 0;

	while (main_exitcall) {
		queueKey = keyQueue->get();
		if (queueKey != 0 || Nullsend > 100) {
			if (queueKey == 1) keyName = USBread_RIGHT;
			else if (queueKey == 2) keyName = USBread_LEFT;
			else if (queueKey == 3) keyName = USBread_SUCCESS;
			else if (queueKey == 4) keyName = USBread_ERROR;
			else if (queueKey == 5) keyName = USBread_NODATA;
			else keyName = 0;

			if (send(MainSock, &keyName, 1, 0) == SOCKET_ERROR) {
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
