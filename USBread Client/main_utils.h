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

struct ipData {
private:
	char* allIp;
	int offset = 0;

public:
	int initIp() {
		HANDLE pipeA = INVALID_HANDLE_VALUE;
		HANDLE pipeAwrite = INVALID_HANDLE_VALUE;
		SECURITY_ATTRIBUTES attr;
		ZeroMemory(&attr, sizeof(SECURITY_ATTRIBUTES));
		attr.nLength = sizeof(SECURITY_ATTRIBUTES);
		attr.bInheritHandle = TRUE;

		if (CreatePipe(&pipeA, &pipeAwrite, &attr, 0) == 0) { return 1; }
		if (!SetHandleInformation(pipeA, HANDLE_FLAG_INHERIT, 0)) { return 1; }

		STARTUPINFOA sInfo;
		ZeroMemory(&sInfo, sizeof(STARTUPINFOA));
		sInfo.cb = sizeof(STARTUPINFOA);
		sInfo.dwFlags = STARTF_USESTDHANDLES;
		sInfo.hStdOutput = pipeAwrite;
		PROCESS_INFORMATION pInfo;
		ZeroMemory(&pInfo, sizeof(PROCESS_INFORMATION));
		if (!CreateProcessA(NULL, (LPSTR)"arp -a", nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &sInfo, &pInfo)) { return 2; }
		WaitForSingleObject(pInfo.hProcess, INFINITE);
		CloseHandle(pInfo.hProcess);
		CloseHandle(pInfo.hThread);
		CloseHandle(pipeAwrite);

		char* rBuffer = (char*)malloc(32 * sizeof(char));
		if (rBuffer == nullptr) { return 3; }
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
				if (rBuffer == nullptr) { return 3; }
			}
			else {
				memset(rBuffer + nReaded, 0, 1);
				break;
			}
		}
		CloseHandle(pipeA);

		//filter all dynamics

		//for (int i = 0; rBuffer[i] != NULL; i++) {
		//	if (rBuffer[i] == '\r') { printf("*"); }
		//	else { printf("%c", rBuffer[i]); }
		//}

		free(rBuffer);

		return 0;
	};

	char* getIp() {
		char ipnow[25];
		int i;
		for (i = 0; allIp[offset + i] != ' '; i++) {
			if (allIp[offset] == '\0') { return NULL; }
			ipnow[i] = allIp[offset + i];
		}
		offset += i + 1;
		return ipnow;
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