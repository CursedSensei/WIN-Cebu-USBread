#pragma once

// Add argument to remove time condition
int main_client() {
    SOCKET MainSock = INVALID_SOCKET;
    while (1) {
	    MainSock = initSocket();
	    if (MainSock == INVALID_SOCKET) {
            time_t nTime = time(NULL);
			struct tm aTime = { };
			localtime_s(&aTime, &nTime);

            if (aTime.tm_wday == 0 && aTime.tm_hour < 12) {
                Sleep(2000);
            }
            else return 1;
        }
        else break;
    }

	SOCKET *youthSock = (SOCKET*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SOCKET));
	*youthSock = MainSock;
	HANDLE youthHandle = CreateThread(NULL, 0, youthThread, (void*)youthSock, 0, NULL);
	if (!youthHandle) {
		closesocket(MainSock);
		HeapFree(GetProcessHeap(), NULL, youthSock);
		WSACleanup();
		return 3;
	}
	CloseHandle(youthHandle);

	queue *keyQueue = (queue *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(queue));
	HANDLE keyhandle = CreateThread(NULL, 0, keyThread, (void*)keyQueue, 0, NULL);
	if (!keyhandle) {
		closesocket(MainSock);
		HeapFree(GetProcessHeap(), NULL, keyQueue);
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
