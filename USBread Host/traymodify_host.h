#pragma once

NOTIFYICONDATAA nid = {};

void addtray(HINSTANCE hInst, HWND hwd) {
	nid.cbSize = sizeof(NOTIFYICONDATAA);
	nid.hWnd = hwd;
	nid.uFlags = NIF_TIP | NIF_MESSAGE | NIF_ICON;
	nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
	strcpy_s(nid.szTip, "WIN Cebu Key Capture Host - Not connected");
	nid.uCallbackMessage = TRAY_CALLBACK;

	Shell_NotifyIconA(NIM_ADD, &nid);
}

void remtray() { Shell_NotifyIconA(NIM_DELETE, &nid); }

void modtray(int mode) {
	if (mode == 2) {
		strcpy_s(nid.szTip, "WIN Cebu Key Capture Host - Not connected");
		strcpy_s(sockStatus, " - Not connected");
	}
	else if (mode) {
		strcpy_s(nid.szTip, "WIN Cebu Key Capture Host");
		strcpy_s(sockStatus, "");
	}
	else { strcpy_s(nid.szTip, "WIN Cebu Key Capture Host - Paused"); }
	Shell_NotifyIconA(NIM_MODIFY, &nid);
}