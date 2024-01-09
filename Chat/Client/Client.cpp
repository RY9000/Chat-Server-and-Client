// Client.cpp

// #define TEST

#pragma comment(lib,"Shcore.lib")


#include "ClientCommon.h"
#include "ClientUtility.h"
#include "ShellScalingAPI.h"



INT_PTR CALLBACK mainDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	wchar_t txt[MAX_NICKNAME + 50];

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		SetWindowText(hwndDlg, g_nickname);
		showMsg(hwndDlg, L"***   Connecting   ***");
		g_networking.connect(SERVER_IP, LISTEN_PORT, hwndDlg);
		g_networking.setNickname(g_nickname);
		return true;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
		{
			EndDialog(hwndDlg, wParam);
			g_networking.exit();
			return true;
		}
		case IDC_SEND:
		{
			wchar_t *textBuffer = nullptr;
			if (getText(hwndDlg, &textBuffer))
			{
				showMsg(hwndDlg, textBuffer);
				sendMsg(textBuffer);
			}
			SetFocus(GetDlgItem(hwndDlg, IDC_INPUT));
			free(textBuffer);
			return true;
		}
		case IDC_INPUT:
		{
			if (GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState(VK_RETURN) && MINSHORT)
			{
				if (EN_UPDATE == HIWORD(wParam))
				{
					wchar_t *textBuffer = nullptr;
					if (getText(hwndDlg, &textBuffer))
					{
						textBuffer[wcslen(textBuffer) - 2] = L'\0';
						showMsg(hwndDlg, textBuffer);
						sendMsg(textBuffer);
					}
					free(textBuffer);
				}
			}
			return true;
		}
		}
		return false;
	}
	case (UINT)net::ClientSysMsg::TEXT:
	{
		showMsg(hwndDlg, (wchar_t *)lParam);
		free((BYTE *)lParam);
		return true;
	}
	case (UINT)net::ClientSysMsg::CONNECTED:
	{
		showMsg(hwndDlg, L"***   Connected. Welcome :)   ***");
		EnableWindow(GetDlgItem(hwndDlg, IDC_INPUT), TRUE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_SEND), TRUE);
		addParticipant(hwndDlg, g_nickname);
		return true;
	}
	case (UINT)net::ClientSysMsg::NEW:
	{
		swprintf_s(txt, MAX_NICKNAME + 50, L"***   %ls has joined us.   ***", (wchar_t *)lParam);
		showMsg(hwndDlg, txt);
		addParticipant(hwndDlg, (wchar_t *)lParam);
		free((BYTE *)lParam);
		return true;
	}
	case (UINT)net::ClientSysMsg::UPDATE_LIST:
	{
		updateList(hwndDlg, (wchar_t *)lParam);
		free((BYTE *)lParam);
		return true;
	}
	case (UINT)net::ClientSysMsg::LEAVE:
	{
		swprintf_s(txt, MAX_NICKNAME + 50, L"***   %ls has left.   ***", (wchar_t *)lParam);
		showMsg(hwndDlg, txt);
		delParticipant(hwndDlg, (wchar_t *)lParam);
		free((BYTE *)lParam);
		return true;
	}
	}
	return false;
}


INT_PTR CALLBACK setNameDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			EndDialog(hwndDlg, false);
			return true;

		case IDOK:
			GetDlgItemText(hwndDlg, IDC_EDIT1, g_nickname, MAX_NICKNAME);
			g_nicknameLen = wcslen(g_nickname);
			if (g_nicknameLen == 0)
			{
				MessageBox(hwndDlg, L"Please enter your nickname.", NULL, MB_OK);
				return true;
			}
			EndDialog(hwndDlg, wParam);
			return true;
		}
		return false;
	}

	return false;
}



int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);

	#ifdef TEST
	wcscpy_s(g_nickname, 48, GetCommandLine());
	g_nicknameLen = wcslen(g_nickname);
	#else
	if (!DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_NICKNAME), NULL, setNameDialogProc))
		return 0;
	#endif // TEST

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, mainDialogProc);
	return 0;
}
