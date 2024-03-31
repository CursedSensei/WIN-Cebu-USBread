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



SOCKET connSocket() {
	addrinfo sockaddr;
	PADDRINFOA hostinfo = NULL;

	ZeroMemory(&sockaddr, sizeof(sockaddr));
	sockaddr.ai_family = AF_INET;
	sockaddr.ai_socktype = SOCK_STREAM;
	sockaddr.ai_protocol = IPPROTO_TCP;
	sockaddr.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, DEFAULT_PORT, &sockaddr, &hostinfo)) return INVALID_SOCKET;

	SOCKET MainSock = INVALID_SOCKET;
	MainSock = socket(hostinfo->ai_family, hostinfo->ai_socktype, hostinfo->ai_protocol);
	if (MainSock == INVALID_SOCKET) {
		freeaddrinfo(hostinfo);
		return INVALID_SOCKET;
	}

	if (bind(MainSock, hostinfo->ai_addr, (int)hostinfo->ai_addrlen) == SOCKET_ERROR) {
		freeaddrinfo(hostinfo);
		closesocket(MainSock);
		return INVALID_SOCKET;
	}

	freeaddrinfo(hostinfo);

	if (listen(MainSock, SOMAXCONN) == SOCKET_ERROR) {
		closesocket(MainSock);
		return INVALID_SOCKET;
	}

	SOCKET ClientSock = INVALID_SOCKET;
	do {
		ClientSock = accept(MainSock, NULL, NULL);
		if (ClientSock == INVALID_SOCKET) {
			break;
		}

		unsigned long long pass = 0;

		if (recv(ClientSock, (char*)&pass, 1, 0) <= 0) {
			closesocket(ClientSock);
			ClientSock = INVALID_SOCKET;
			continue;
		}

		if (pass != USBread_INCOMP) {
			closesocket(ClientSock);
			ClientSock = INVALID_SOCKET;
			continue;
		}

		if (send(ClientSock, (char*)&pass, 8, 0) == SOCKET_ERROR) {
			closesocket(ClientSock);
			ClientSock = INVALID_SOCKET;
			continue;
		}

		pass = 0;

		if (recv(ClientSock, (char*)&pass, 1, 0) <= 0) {
			closesocket(ClientSock);
			ClientSock = INVALID_SOCKET;
			continue;
		}

		if (pass != USBread_COMP) {
			closesocket(ClientSock);
			ClientSock = INVALID_SOCKET;
		}
	} while (ClientSock == INVALID_SOCKET);

	closesocket(MainSock);

	return ClientSock;
}

SOCKET initSocket() {
	WSADATA wsData;
	if (WSAStartup(MAKEWORD(2, 2), &wsData)) {
		return 1;
	}

	updateIp();

	SOCKET ClientSock = connSocket();
	if (ClientSock == INVALID_SOCKET) WSACleanup();

	return ClientSock;
}



int main_client(HWND hWnd) {
	ClientSock = initSocket();
	if (ClientSock == INVALID_SOCKET) return 1;

	int sockstatus;
	char keyName;
	int keyId;

	modtray(1);
	InvalidateRect(hWnd, NULL, TRUE);

	main_exitcall = 1;

	do {
		if (!main_exitcall) { break; }

		sockstatus = recv(ClientSock, &keyName, 1, 0);
		if (sockstatus > 0) {
			if (main_exitcall == 2) { continue; }

			if (keyName == USBread_RIGHT) {
				PostMessage(hWnd, WM_KEYDOWN, VK_RIGHT, 0);
			}
			else if (keyName == USBread_LEFT) {
				PostMessage(hWnd, WM_KEYDOWN, VK_LEFT, 0);
			}
			else if (keyName == USBread_SUCCESS) {
				notiftray("Youth Poster successfully downloaded");
			}
			else if (keyName == USBread_ERROR) {
				notiftray("Youth Poster failed to download");
			}
			else { continue; }
		}
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
		ClientSock = INVALID_SOCKET;
		InvalidateRect(*(HWND*)args, NULL, TRUE);
	}

	return status;
}