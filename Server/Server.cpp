//

#include "common.h"


std::unordered_map<std::string, std::pair<UINT,int>> cmdMap;

DWORD IDofMainThread;

const wchar_t *helpInfo = L"Commands:\n"
                          L"\t-start\t\tStart the server.\n"
                          L"\t-stop\t\tStop the server.\n";



void initCmd()
{
	cmdMap["-help"] = { srvCmd_Help,0 };
	cmdMap["-stop"] = { srvCmd_Stop,0 };
	cmdMap["-start"] = { srvCmd_Start,0 };
}



void input()
{
    std::string cmdLine;

    while (1)
    {
        getline(std::cin, cmdLine);

        size_t start = 0;
        size_t end = cmdLine.find(" ");

        std::string name = cmdLine.substr(start, end - start);

        if (cmdMap.count(name) == 0)
        {
            std::cout << "[Server] Unrecognized command \"" << name << "\".\n";
            continue;
        }

        std::vector<std::string> *p = new std::vector<std::string>();

        while (end != -1) 
        {
            start = end + 1;
            end = cmdLine.find(" ", start);
            if (cmdLine.substr(start, end - start) != "")
            {
                p->push_back(cmdLine.substr(start, end - start));
            }
        }

        if (cmdMap[name].second != p->size())
        {
            std::cout << "[Server] Arguments Error.\n";
            p->clear();
            delete p;
            continue;
        }

        PostThreadMessage(IDofMainThread, cmdMap[name].first, 0, (LPARAM)p);
    }
}


int main()
{
    IDofMainThread = GetCurrentThreadId();

    initCmd();

    std::thread inputThread(input);
 

    MSG msg;
    
    net::Server networking(56565, 6);

    std::vector<std::string> *pArgu;

    while (1)
    {
        GetMessage(&msg, NULL, 0, 0);

        switch (msg.message)
        {
        case srvCmd_Start: 
            pArgu = (std::vector<std::string>*)msg.lParam;
            networking.Start();
            pArgu->clear();
            delete pArgu;
            break;

        case srvCmd_Stop:
            pArgu = (std::vector<std::string>*)msg.lParam;
            networking.Stop();
            pArgu->clear();
            delete pArgu;
            break;

        case srvCmd_Help:
            pArgu = (std::vector<std::string>*)msg.lParam;
            printf("%ls\n", helpInfo);
            pArgu->clear();
            delete pArgu;
            break;
        }
    }


    return 0;
}

