
#include "ServerCommon.h"
#include "windows.h"


const int MAX_ARGU_LEN = 20;


const char *g_helpInfo = "One command and its arguments each time.\n"
"Commands:\n"
"\t-help \t\t\tShow the manual.\n"
"\t-start <number> \tStart the server with <number> threads in thread pool.\n"
"\t-stop \t\t\tStop the server.\n";


enum class Commands:int
{
	HELP, START, STOP
};


std::unordered_map<std::string, std::pair<Commands, int>> g_cmdMap; // input, Commands, number of its arguments


void initCmdMap()
{
	g_cmdMap["-help"] = { Commands::HELP,0 };
	g_cmdMap["-stop"] = { Commands::STOP,0 };
	g_cmdMap["-start"] = { Commands::START,1 }; // one argument: number of threads in thread pool
}


int main()
{
	SetConsoleTitle(L"Chat Server");

	initCmdMap();

	net::Server networking;

	std::string cmdLine;

	bool running = true;

	while (running)
	{
		getline(std::cin, cmdLine);

		std::stringstream ss(cmdLine);
		std::string cmd;

		ss >> cmd;

		if (cmd == "")
			continue;

		if (g_cmdMap.count(cmd) == 0)
		{
			std::cout << "Unrecognized command \"" << cmd << "\"." << std::endl;
			continue;
		}

		std::string argu;
		std::vector<std::string> argus;
		bool noError = true;

		while (ss >> argu)
		{
			if (argu.size() > MAX_ARGU_LEN)
			{
				noError = false;
				break;
			}
			argus.push_back(argu);
		}

		if (!noError || argus.size() != g_cmdMap[cmd].second)
		{
			std::cout << "Arguments Error." << std::endl;
			continue;
		}


		switch (g_cmdMap[cmd].first)
		{
		case Commands::START:
		{
			if (argus[0].find_first_not_of("0123456789") != std::string::npos)
			{
				std::cout << "Arguments Error." << std::endl;
			}
			else
			{
				networking.start(stoi(argus[0]));
			}
			break;
		}
		case Commands::STOP:
		{
			networking.stop();
			break;
		}
		case Commands::HELP:
		{
			std::cout << g_helpInfo << std::endl;
			break;
		}
		}
	}
	return 0;
}
