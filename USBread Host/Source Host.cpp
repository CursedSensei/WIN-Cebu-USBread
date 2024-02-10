#include "Frameworks_Host.h"
#include "Globals.h"
#include "InitFunc.h"
#include "traymodify_host.h"
#include "main_host.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

_Use_decl_annotations_ int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PWSTR cmdline, int cmdshow) {

	UNREFERENCED_PARAMETER(hInstPrev);
	UNREFERENCED_PARAMETER(cmdline);

	const wchar_t classname[] = L"USBRead Host CLASS";

	HWND previnst = FindWindow(classname, NULL);
	if (previnst != NULL) {
		CloseHandle(previnst);
		return 0;
	}

	WNDCLASS wc = { };
	wc.hInstance = hInst;
	wc.lpszClassName = classname;
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
	wc.lpfnWndProc = WndProc;

	RegisterClass(&wc);

	globalInitialize(0);
	if (FAILED(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED))) { return -1; };

	HWND hwd = CreateWindowEx(
		0,
		classname,
		L"WIN Cebu Key Capture Host",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		100,
		100,
		1280,
		720,
		NULL,
		NULL,
		hInst,
		NULL);

	if (hwd == NULL) {
		return -1;
	}

	HWND* buffered = (HWND*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HWND));
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
	HeapFree(GetProcessHeap(), NULL, buffered);

	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		return 0;
	}
	resetIp();
	WSACleanup();
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		{
			switch (wParam) {

			case VK_RIGHT:
				curNum++;
				break;
			case VK_LEFT:
				curNum--;
				break;
			default:
				return DefWindowProc(hWnd, uMsg, wParam, lParam);
			}
			if (curNum == 0) { curNum++; }
			else if (curNum > maxNum) {
				if (maxNum == 0) { return DefWindowProc(hWnd, uMsg, wParam, lParam); }
				curNum--;
			}
			else { InvalidateRect(hWnd, NULL, TRUE); }
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			RECT client;
			GetClientRect(hWnd, &client);
			if (maxNum) {
				HDC memhdc = CreateCompatibleDC(hdc);
				char fullPath[MAX_PATH];
				char picname[20];
				snprintf(picname, 20, "%d.bmp", curNum);
				snprintf(fullPath, MAX_PATH, bmpDirectory, picname);
				HBITMAP backgroud = (HBITMAP)LoadImageA(NULL, fullPath, IMAGE_BITMAP, client.right, client.bottom, LR_LOADFROMFILE);
				SelectObject(memhdc, backgroud);
				StretchBlt(hdc, 0, 0, client.right, client.bottom, memhdc, 0, 0, client.right, client.bottom, SRCCOPY);

				DeleteDC(memhdc);
				DeleteObject(backgroud);
			}
			else { FillRect(hdc, &client, (HBRUSH)COLOR_WINDOW + 1); }
			EndPaint(hWnd, &ps);

			char winText[75];
			if (maxNum == 0) {
				snprintf(winText, 75, "WIN Cebu Key Capture Host (No Slides)%s", sockStatus);
				SetWindowTextA(hWnd, winText);
			}
			else {
				snprintf(winText, 75, "WIN Cebu Key Capture Host (Slide: %d/%d)%s", curNum, maxNum, sockStatus);
				SetWindowTextA(hWnd, winText);
			}
		}
		break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case ID_FILE_OPENDIRECTORY:
				openDirectory(hWnd);
				break;
			case ID_FILE_CHANGEDIRECTORY:
				{
					char winText[75];
					snprintf(winText, 75, "Initializing Slides%s", sockStatus);
					SetWindowTextA(hWnd, winText);
					changeFiles();
					globalInitialize(1);
					InvalidateRect(hWnd, NULL, TRUE);
				}
				break;
			case ID_FILE_ADDYOUTHPOSTER:
				{
					if (ClientSock != INVALID_SOCKET) {
						unsigned char code = USBread_YOUTH;
						send(ClientSock, (char*)&code, 1, 0);
					}
				}
				break;
			case ID_SETTINGS_PAUSE:
				{
					if (main_exitcall == 2) {
						unpauseKey(hWnd);
					}
					else if (main_exitcall == 1) {
						pauseKey(hWnd);
					}
				}
				break;
			case ID_SETTINGS_EXIT:
				DestroyWindow(hWnd);
				break;
			default:
				return DefWindowProc(hWnd, uMsg, wParam, lParam);
			}
		}
		break;
	case TRAY_CALLBACK:
		switch (lParam)
		{
		case WM_LBUTTONDBLCLK: {
				main_exitcall = 0;
				DestroyWindow(hWnd);
			}
			break;
		case WM_RBUTTONDOWN: {
				if (main_exitcall == 3) { break; }

				if (main_exitcall == 2) {
					unpauseKey(hWnd);
				}
				else if (main_exitcall == 1) {
					pauseKey(hWnd);
				}
			}
			break;
		}
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}