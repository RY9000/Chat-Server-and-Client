#pragma once

#include "..\Networking\net_client.h"
#include "client_common.h"
#include "cli_msg.h"


#define MAX_Nickname	50
wchar_t Nickname[MAX_Nickname];
size_t  nicknameLen = 0;


static size_t textBufferSize = 0;
static wchar_t *textBuffer = NULL;


net::Client networking;
#define serverIP		"127.0.0.1"
#define serverPort		56565


/*
   wchar_t time[21] = L"\0";
   GetDateandTime(time, 21);
*/
void GetDateandTime(wchar_t *s, size_t size)
{
	SYSTEMTIME lt;
	GetLocalTime(&lt);
	wchar_t month[4] = L"\0";
	switch (lt.wMonth)
	{
	case 1:
		wcscpy_s(month, 4, L"Jan");
		break;
	case 2:
		wcscpy_s(month, 4, L"Feb");
		break;
	case 3:
		wcscpy_s(month, 4, L"Mar");
		break;
	case 4:
		wcscpy_s(month, 4, L"Apr");
		break;
	case 5:
		wcscpy_s(month, 4, L"May");
		break;
	case 6:
		wcscpy_s(month, 4, L"Jun");
		break;
	case 7:
		wcscpy_s(month, 4, L"Jul");
		break;
	case 8:
		wcscpy_s(month, 4, L"Aug");
		break;
	case 9:
		wcscpy_s(month, 4, L"Sep");
		break;
	case 10:
		wcscpy_s(month, 4, L"Oct");
		break;
	case 11:
		wcscpy_s(month, 4, L"Nov");
		break;
	case 12:
		wcscpy_s(month, 4, L"Dec");
		break;
	}
	swprintf_s(s, size, L"%ls-%02d-%04d %02d:%02d:%02d", month, lt.wDay, lt.wYear,
			   lt.wHour, lt.wMinute, lt.wSecond);

}


void ShowMsg(HWND hwndDlg, const wchar_t *s)
{
	wchar_t time[27] = L"-- ";
	GetDateandTime(time+3, 21);
	wcscat_s(time, 27, L" --");

	HWND hEdit = GetDlgItem(hwndDlg, IDC_OUTPUT);

	size_t index = GetWindowTextLength(hEdit);
	SendMessage(hEdit, EM_SETSEL, (WPARAM)index, (LPARAM)index);
	SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)time);

	wchar_t newLine[3] = { 0x0d,0x0a,L'\0' };

	index += 26;
	SendMessage(hEdit, EM_SETSEL, (WPARAM)index, (LPARAM)index);
	SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)newLine);

	index += 2;
	SendMessage(hEdit, EM_SETSEL, (WPARAM)index, (LPARAM)index);
	SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)s);

	index += wcslen(s);
	SendMessage(hEdit, EM_SETSEL, (WPARAM)index, (LPARAM)index);
	SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)newLine);

	index += 2;
	SendMessage(hEdit, EM_SETSEL, (WPARAM)index, (LPARAM)index);
	SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)newLine);
}


bool GetText(HWND hwndDlg)
{
	HWND hControl= GetDlgItem(hwndDlg, IDC_INPUT);
	size_t length = SendMessage((HWND)hControl, WM_GETTEXTLENGTH, 0, 0);
	if (length<=0)
	{
		SendMessage((HWND)hControl, WM_SETTEXT, 0, (LPARAM)L"");
		MessageBox(hControl, L"Please enter your message.", NULL, MB_OK);
		return false;
	}

	if (textBufferSize < (length + nicknameLen + 4))
	{
		textBufferSize = length + nicknameLen + 4;
		textBuffer = (wchar_t *)realloc(textBuffer, textBufferSize * sizeof(wchar_t));
	}
	swprintf_s(textBuffer, textBufferSize, L"[%ls] ", Nickname);
	SendMessage((HWND)hControl, WM_GETTEXT, length + 1, (LPARAM)(textBuffer + nicknameLen + 3));
	SendMessage((HWND)hControl, WM_SETTEXT, 0, (LPARAM)L"");

	for (size_t i = nicknameLen + 3; i < length + nicknameLen + 3; i++)
	{
		if (textBuffer[i] != 0x0d && textBuffer[i] != 0x0a && textBuffer[i] != L' ')
			return true;
	}
	
	MessageBox(hControl, L"Please enter your message.", NULL, MB_OK);
	return false;
}


void SendMsg(wchar_t *p)
{
	net::message msg;
	msg.header.ID = net::msgID::text;
	msg.FillData(wcslen(p) * sizeof(wchar_t), (uint8_t *)p);
	networking.SendMsg(msg);
}


void AddParticipant(HWND hwndDlg, wchar_t *name)
{
	SendDlgItemMessage(hwndDlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM)name);
}


void DelParticipant(HWND hwndDlg, wchar_t *name)
{
	size_t index = SendDlgItemMessage(hwndDlg, IDC_LIST, LB_FINDSTRING, -1, (LPARAM)name);

	if(index!= LB_ERR)
		SendDlgItemMessage(hwndDlg, IDC_LIST, LB_DELETESTRING, index, 0);
}


void updateList(HWND hwndDlg, wchar_t *p)
{
	wchar_t nn[MAX_Nickname];
	
	for (int m = 0, n = 0; p[m] != L'\0'; m++)
	{
		if (p[m] != L'\t')
		{
			nn[n] = p[m];
			n++;
		}
		else
		{
			nn[n] = L'\0';
			AddParticipant(hwndDlg, nn);
			n = 0;
		}
	}
}
