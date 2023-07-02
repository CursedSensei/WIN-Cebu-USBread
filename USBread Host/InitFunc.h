#pragma once

int EndsWith(const char* str, const char* suffix)
{
	if (!str || !suffix)
		return 0;
	size_t lenstr = strlen(str);
	size_t lensuffix = strlen(suffix);
	if (lensuffix > lenstr)
		return 0;
	return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

int getFileNum(char* fileName) {
	int num = 0;
	int maxBuf = strlen(fileName);
	if (fileName[0] != 83) {
		num = (maxBuf - 5) * 26;
		num += fileName[maxBuf - 5] - 96;
	}
	else {
		int digits = maxBuf - 9;
		int pow = 1;
		for (int i = 1; i < digits; i++) {
			pow = pow * 10;
		}
		for (int i = 5; pow >= 1; i++) {
			num += pow * (fileName[i] - 48);
			pow = pow / 10;
		}
	}
	return num;
}

void globalInitialize(int mode) {
	if (!mode) {
		GetCurrentDirectoryA(MAX_PATH, bmpDirectory);
		strcat_s(bmpDirectory, sizeof(bmpDirectory), "\\Cache\\%s");
	}
	char path[MAX_PATH];
	snprintf(path, MAX_PATH, bmpDirectory, "**");
	maxNum = 0;
	curNum = 1;

	WIN32_FIND_DATAA fileData;
	HANDLE hFile;
	char delFile[MAX_PATH];

	hFile = FindFirstFileA(path, &fileData);
	if (hFile == INVALID_HANDLE_VALUE) {
		return;
	}
	do {
		if (EndsWith(fileData.cFileName, ".bmp") || EndsWith(fileData.cFileName, ".BMP"))
		{
			if (mode == 2) {
				snprintf(delFile, MAX_PATH, bmpDirectory, fileData.cFileName);
				DeleteFileA(delFile);
			}
			else { maxNum++; }
		}
	} while (FindNextFileA(hFile, &fileData) != 0);
	FindClose(hFile);

	return;
}

void openDirectory(HWND hWnd) {
	char configPath[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, configPath);
	strcat_s(configPath, sizeof(configPath), "\\config.cfg");
	FILE* conf;
	if (fopen_s(&conf, configPath, "r") == 0) {
		char path[MAX_PATH];
		char explorerCommand[MAX_PATH + 10];

		fgets(path, MAX_PATH, conf);
		fclose(conf);

		if (strlen(path) == 0) {
			MessageBoxA(hWnd, "Directory Failed", "Directory not found.", MB_ICONERROR | MB_OK);
			return;
		}

		snprintf(explorerCommand, MAX_PATH + 10, "explorer \"%s\"", path);
		STARTUPINFOA siFile;
		ZeroMemory(&siFile, sizeof(STARTUPINFOA));
		siFile.cb = sizeof(STARTUPINFOA);
		PROCESS_INFORMATION piFile;
		ZeroMemory(&piFile, sizeof(PROCESS_INFORMATION));
		CreateProcessA(NULL, explorerCommand, 0, 0, FALSE, CREATE_NO_WINDOW, NULL, NULL, &siFile, &piFile);
		CloseHandle(piFile.hProcess);
		CloseHandle(piFile.hThread);
		return;
	}
	MessageBoxA(hWnd, "Directory Failed", "No Directory.", MB_ICONERROR | MB_OK);

	return;
}

void changeFiles() {
	WCHAR* wpath;
	char path[MAX_PATH];
	IFileDialog* fileDiag;
	if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fileDiag)))) { return; }
	DWORD options;
	fileDiag->GetOptions(&options);
	fileDiag->SetOptions(options | FOS_PICKFOLDERS);
	if (fileDiag->Show(NULL) != S_OK) { return; }
	IShellItem* si;
	fileDiag->GetResult(&si);
	si->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &wpath);
	size_t maxP = MAX_PATH;
	wcstombs_s(&maxP, path, sizeof(char) * MAX_PATH, wpath, sizeof(WCHAR) * MAX_PATH);
	si->Release();

	char configPath[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, configPath);
	strcat_s(configPath, sizeof(configPath), "\\config.cfg");

	FILE* conf;
	if (fopen_s(&conf, configPath, "w") == 0) {
		fprintf_s(conf, path);
		fclose(conf);
	}

	globalInitialize(2);

	char comFormat[MAX_PATH * 2 + 50];
	snprintf(comFormat, MAX_PATH * 2 + 50, bmpDirectory, "");

	if (!(CreateDirectoryA(comFormat, NULL) || ERROR_ALREADY_EXISTS == GetLastError())) { return; }

	snprintf(comFormat, MAX_PATH * 2 + 50, "FFmpeg\\bin\\ffmpeg -i \"%s\\%s\" \"%s%s.bmp\"", path, "%s", bmpDirectory, "%d");
	strcat_s(path, sizeof(path), "\\**");

	WIN32_FIND_DATAA fileData;
	HANDLE hFile;
	
	hFile = FindFirstFileA(path, &fileData);
	if (hFile == INVALID_HANDLE_VALUE) {
		return;
	}

	char fileCommand[MAX_PATH * 2 + 50];
	STARTUPINFOA siFile;
	ZeroMemory(&siFile, sizeof(STARTUPINFOA));
	siFile.cb = sizeof(STARTUPINFOA);
	PROCESS_INFORMATION piFile[150];
	ZeroMemory(&piFile, sizeof(piFile));
	int curProc = 0;

	do {
		if (EndsWith(fileData.cFileName, ".png") || EndsWith(fileData.cFileName, ".PNG"))
		{
			snprintf(fileCommand, MAX_PATH * 2 + 50, comFormat, fileData.cFileName, "", getFileNum(fileData.cFileName));
			CreateProcessA(NULL, fileCommand, 0, 0, FALSE, CREATE_NO_WINDOW, NULL, NULL, &siFile, &piFile[curProc]);
			CloseHandle(piFile[curProc].hThread);
			curProc++;
		}
	} while (FindNextFileA(hFile, &fileData) != 0);
	FindClose(hFile);
	for (curProc = 0; piFile[curProc].hProcess != NULL; curProc++) {
		WaitForSingleObject(piFile[curProc].hProcess, INFINITE);
		CloseHandle(piFile[curProc].hProcess);
	}

	return;
}