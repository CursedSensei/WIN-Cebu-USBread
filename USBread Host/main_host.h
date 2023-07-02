#pragma once

DWORD WINAPI accelThread(LPVOID args) {
	int Lbuffer = 0;
	int Rbuffer = 0;
	int i;
	while (1) {
		if (HIBYTE(GetKeyState(VK_LCONTROL))) {
			i = HIBYTE(GetKeyState(VK_RIGHT));
			if (Rbuffer != i) {
				Rbuffer = i;
				if (i != 0) { PostMessage(*(HWND*)args, WM_KEYDOWN, VK_RIGHT, 0); }
			}
			i = HIBYTE(GetKeyState(VK_LEFT));
			if (Lbuffer != i) {
				Lbuffer = i;
				if (i != 0) { PostMessage(*(HWND*)args, WM_KEYDOWN, VK_LEFT, 0); }
			}
		}
		Sleep(30);
	}
	return 0;
}

void pauseKey(HWND hWnd) {
	main_exitcall = 2;
	modtray(0);

	MENUITEMINFO iMenu;
	ZeroMemory(&iMenu, sizeof(MENUITEMINFO));
	iMenu.cbSize = sizeof(MENUITEMINFO);
	iMenu.fMask = MIIM_STATE;
	GetMenuItemInfo(GetMenu(hWnd), ID_SETTINGS_PAUSE, FALSE, &iMenu);
	iMenu.fState = MFS_CHECKED;
	SetMenuItemInfo(GetMenu(hWnd), ID_SETTINGS_PAUSE, FALSE, &iMenu);

	return;
}

void unpauseKey(HWND hWnd) {
	main_exitcall = 1;
	modtray(1);

	MENUITEMINFO iMenu;
	ZeroMemory(&iMenu, sizeof(MENUITEMINFO));
	iMenu.cbSize = sizeof(MENUITEMINFO);
	iMenu.fMask = MIIM_STATE;
	GetMenuItemInfo(GetMenu(hWnd), ID_SETTINGS_PAUSE, FALSE, &iMenu);
	iMenu.fState = MFS_UNCHECKED;
	SetMenuItemInfo(GetMenu(hWnd), ID_SETTINGS_PAUSE, FALSE, &iMenu);

	return;
}

int checkKey(char* key) {
	char buf[6];
	int status = 0;

	strcpy_s(buf, "Right");
	for (int i = 0; buf[i] != NULL; i++) {
		if (key[i] != buf[i]) {
			status = 1;
			break;
		}
	}
	if (status == 0) { return 1; }
	else { status = 0; }

	strcpy_s(buf, "Left ");
	for (int i = 0; buf[i] != NULL; i++) {
		if (key[i] != buf[i]) {
			status = 1;
			break;
		}
	}
	if (status == 0) { return 2; }
	else { return 0; }
}

int main_client(HWND hWnd) {
	WSADATA wsData;
	if (WSAStartup(MAKEWORD(2, 2), &wsData)) {
		return 1;
	}

	addrinfo sockaddr;
	PADDRINFOA hostinfo = NULL;

	ZeroMemory(&sockaddr, sizeof(sockaddr));
	sockaddr.ai_family = AF_INET;
	sockaddr.ai_socktype = SOCK_STREAM;
	sockaddr.ai_protocol = IPPROTO_TCP;
	sockaddr.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, DEFAULT_PORT, &sockaddr, &hostinfo)) {
		WSACleanup();
		return 2;
	}

	SOCKET MainSock = INVALID_SOCKET;
	MainSock = socket(hostinfo->ai_family, hostinfo->ai_socktype, hostinfo->ai_protocol);
	if (MainSock == INVALID_SOCKET) {
		freeaddrinfo(hostinfo);
		WSACleanup();
		return 3;
	}

	if (bind(MainSock, hostinfo->ai_addr, (int)hostinfo->ai_addrlen) == SOCKET_ERROR) {
		freeaddrinfo(hostinfo);
		closesocket(MainSock);
		WSACleanup();
		return 3;
	}

	freeaddrinfo(hostinfo);

	if (listen(MainSock, SOMAXCONN) == SOCKET_ERROR) {
		closesocket(MainSock);
		WSACleanup();
		return 3;
	}

	SOCKET ClientSock = INVALID_SOCKET;
	ClientSock = accept(MainSock, NULL, NULL);
	if (ClientSock == INVALID_SOCKET) {
		closesocket(MainSock);
		WSACleanup();
		return 3;
	}

	closesocket(MainSock);

	if (shutdown(ClientSock, SD_SEND) == SOCKET_ERROR) {
		closesocket(ClientSock);
		WSACleanup();
		return 3;
	}

	int sockstatus;
	char keyName[6];
	int keyId;

	modtray(1);
	InvalidateRect(hWnd, NULL, TRUE);

	main_exitcall = 1;

	do {
		if (!main_exitcall) { break; }

		sockstatus = recv(ClientSock, keyName, 6, 0);
		if (sockstatus > 0) {
			if (main_exitcall == 2) { continue; }

			keyId = checkKey(keyName);
			if (keyId == 1) {
				PostMessage(hWnd, WM_KEYDOWN, VK_RIGHT, 0);
			}
			else if (keyId == 2) {
				PostMessage(hWnd, WM_KEYDOWN, VK_LEFT, 0);
			}
			else { continue; }
		}
		else if (sockstatus == 0) {}
		else {
			closesocket(ClientSock);
			WSACleanup();
			return 4;
		}
	} while (sockstatus > 0);

	closesocket(ClientSock);
	WSACleanup();
	return 0;
}

DWORD WINAPI client_thread(LPVOID args) {
	int status = 0;

	ShowWindow(*(HWND*)args, SW_NORMAL);

	HANDLE accel_thread = CreateThread(NULL, 0, accelThread, args, 0, NULL);
	if (accel_thread) {
		CloseHandle(accel_thread);
	}

	while (1) {
		status = main_client(*(HWND*)args);
		main_exitcall = 3;
		modtray(2);
		InvalidateRect(*(HWND*)args, NULL, TRUE);
	}

	return status;
}