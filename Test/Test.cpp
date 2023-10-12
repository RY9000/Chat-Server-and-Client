// Test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>

int main()
{
    wchar_t appName[MAX_PATH] = L".\\Client.exe";
    wchar_t CommandLine[99];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    GetStartupInfo(&si);
    si.cb = sizeof(si);


    for (int i = 1; i <= 20; i++)
    {
        _itow_s(i, CommandLine, 99, 10);
        
        CreateProcess(appName, CommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    return 0;
}
