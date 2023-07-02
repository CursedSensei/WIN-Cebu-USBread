#include "Frameworks.h"
#include "traymodify.h"
#include "main.h"

int main() {
	HANDLE pipeA = INVALID_HANDLE_VALUE;
	HANDLE pipeAwrite = INVALID_HANDLE_VALUE;

	if (CreatePipe(&pipeA, &pipeAwrite, NULL, 0) == 0) { return 1; }
	if (!SetHandleInformation(pipeA, HANDLE_FLAG_INHERIT, 0)) { return 1; }

	STARTUPINFOA sInfo;
	ZeroMemory(&sInfo, sizeof(STARTUPINFOA));
	sInfo.cb = sizeof(STARTUPINFOA);
	sInfo.dwFlags = STARTF_USESTDHANDLES;
	sInfo.hStdOutput = pipeAwrite;
	PROCESS_INFORMATION pInfo;
	ZeroMemory(&pInfo, sizeof(PROCESS_INFORMATION));
	if (!CreateProcessA(nullptr, (LPSTR)"arp -a", nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &sInfo, &pInfo)) { return 2; }
	WaitForSingleObject(pInfo.hProcess, INFINITE);
	printf("to reading");

	char rBuf[500];
	DWORD bRead;
	bool reStatus = FALSE;
	for (;;) {
		reStatus = ReadFile(pipeA, rBuf, 500, &bRead, NULL) || bRead != 0;
		if (!reStatus || bRead == 0) continue;
		printf("%s", rBuf);
		if (!reStatus) break;
	}

	CloseHandle(pInfo.hProcess);
	CloseHandle(pInfo.hThread);
	CloseHandle(pipeA);
	CloseHandle(pipeAwrite);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

_Use_decl_annotations_ int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PWSTR cmdline, int cmdshow) { 

	UNREFERENCED_PARAMETER(hInstPrev);
	UNREFERENCED_PARAMETER(cmdline);

	const wchar_t classname[] = L"USBRead Client CLASS";
	const wchar_t windowname[] = L"USBRead Client WINDOW";

	HWND previnst = FindWindow(classname, windowname);
	if (previnst != NULL) {
		CloseHandle(previnst);
		return 0;
	}

	WNDCLASS wc = { };
	wc.hInstance = hInst;
	wc.lpszClassName = classname;
	wc.lpfnWndProc = WndProc;

	RegisterClass(&wc);

	HWND hwd = CreateWindowEx(
		0,
		classname,
		windowname,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInst,
		NULL);
	if (hwd == NULL) {
		return -1;
	}
	HWND *buffered = (HWND *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HWND));
	if (buffered == NULL) {
		return -2;
	}
	*buffered = hwd;

	HANDLE main_thread = CreateThread(NULL, 0, client_thread, buffered, 0, NULL);
	if (!main_thread) {
		return -2;
	}
	CloseHandle(main_thread);

	addtray(hInst, hwd);

	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	remtray();
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case TRAY_CALLBACK:
		switch (lParam)
		{
		case WM_LBUTTONDBLCLK:
			main_exitcall = 0;
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		}
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}