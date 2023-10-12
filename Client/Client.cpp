
#pragma comment(lib,"Shcore.lib")


#include "client_utility.h"
#include "ShellScalingAPI.h"





INT_PTR CALLBACK MainDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	wchar_t txt[MAX_Nickname + 50];

	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetWindowText(hwndDlg, Nickname);
		ShowMsg(hwndDlg, L"***   Connecting   ***");
		networking.Connect(serverIP, serverPort, hwndDlg);
		networking.SetNickname(Nickname);
		return true;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			EndDialog(hwndDlg, wParam);
			networking.SysCmd(cliCmd_exit, 0);
			return TRUE;

		case IDC_SEND:
			if (GetText(hwndDlg))
			{
				ShowMsg(hwndDlg, textBuffer);
				SendMsg(textBuffer);
			}
			SetFocus(GetDlgItem(hwndDlg, IDC_INPUT));
			return true;

		case IDC_INPUT:
			if (GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState(VK_RETURN) && MINSHORT)
			{
				if (EN_UPDATE == HIWORD(wParam))
				{
					if (GetText(hwndDlg))
					{
						textBuffer[wcslen(textBuffer) - 2] = L'\0';
						ShowMsg(hwndDlg, textBuffer);
						SendMsg(textBuffer);
					}
				}
			}
			return true;
		}
		return FALSE;

	case cliCmd_text:
		ShowMsg(hwndDlg, (wchar_t*)lParam);
		free((uint8_t*)lParam);
		return true;

	case cliCmd_connected:
		ShowMsg(hwndDlg, L"***   Connected. Welcome :)   ***");
		EnableWindow(GetDlgItem(hwndDlg, IDC_INPUT), TRUE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_SEND), TRUE);
		AddParticipant(hwndDlg, Nickname);
		return true;

	case cliCmd_newguy:
		swprintf_s(txt, MAX_Nickname + 50, L"***   %ls has joined us.   ***", (wchar_t*)lParam);
		ShowMsg(hwndDlg, txt);
		AddParticipant(hwndDlg, (wchar_t*)lParam);
		free((wchar_t*)lParam);
		return true;

	case cliCmd_updatelist:
		updateList(hwndDlg, (wchar_t*)lParam);
		free((uint8_t*)lParam);
		return true;

	case cliCmd_leave:
		swprintf_s(txt, MAX_Nickname + 50, L"***   %ls has left.   ***", (wchar_t*)lParam);
		ShowMsg(hwndDlg, txt);
		DelParticipant(hwndDlg, (wchar_t*)lParam);
		free((uint8_t*)lParam);
		return true;
	}

	return FALSE;
}


INT_PTR CALLBACK NNDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			EndDialog(hwndDlg, false);
			return TRUE;

		case IDOK:
			GetDlgItemText(hwndDlg, IDC_EDIT1, Nickname, MAX_Nickname);
			nicknameLen = wcslen(Nickname);
			if (nicknameLen == 0)
			{
				MessageBox(hwndDlg, L"Please enter your nickname.", NULL, MB_OK);
				return true;
			}
			EndDialog(hwndDlg, wParam);
			return TRUE;
		}
		return FALSE;
	}

	return FALSE;
}



int APIENTRY wWinMain(_In_     HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_     LPWSTR    lpCmdLine,
	_In_     int       nCmdShow)
{
	SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);

#ifdef TEST
	wcscpy_s(Nickname, 48, GetCommandLine());
	nicknameLen = wcslen(Nickname);
#else
	if (!DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_Nickname), NULL, NNDialogProc))
		return 0;
#endif // TEST


	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, MainDialogProc);

	return 0;
}
