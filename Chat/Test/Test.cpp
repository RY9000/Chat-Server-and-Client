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


    for (int i = 1; i <= 10; i++)
    {
        _itow_s(i, CommandLine, 99, 10);
        
        CreateProcess(appName, CommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        Sleep(10);
    }

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
