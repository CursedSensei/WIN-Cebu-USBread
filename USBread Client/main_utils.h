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

		if (reRun < 2) {

			STARTUPINFOA sInfo;
			ZeroMemory(&sInfo, sizeof(STARTUPINFOA));
			sInfo.cb = sizeof(STARTUPINFOA);
			PROCESS_INFORMATION pInfo;
			ZeroMemory(&pInfo, sizeof(PROCESS_INFORMATION));

			if (reRun == 0) {
				if (!CreateProcessA(NULL, (LPSTR)"ping 192.168.0.185", nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &sInfo, &pInfo)) return INVALID_SOCKET;
			}
			else {
				if (!CreateProcessA(NULL, (LPSTR)"ping 192.168.0.189", nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &sInfo, &pInfo)) return INVALID_SOCKET;
			}
			
			reRun++;

			CloseHandle(pInfo.hThread);
			WaitForSingleObject(pInfo.hProcess, INFINITE);
			CloseHandle(pInfo.hProcess);

			goto Rerun;
		}

		return INVALID_SOCKET;
	}

	if (shutdown(MainSock, SD_RECEIVE) == SOCKET_ERROR) {
		closesocket(MainSock);
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