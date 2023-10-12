#pragma once
#include "client_common.h"

#define cliCmd_connected					WM_USER+3
#define cliCmd_text							cliCmd_connected+1
#define cliCmd_nickname						cliCmd_text+1
#define cliCmd_newguy						cliCmd_nickname+1
#define cliCmd_updatelist					cliCmd_newguy+1
#define cliCmd_exit							cliCmd_updatelist+1
#define cliCmd_leave						cliCmd_exit+1