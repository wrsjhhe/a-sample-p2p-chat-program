#pragma once
#include <windows.h>
#include <cstdio>
#include "Exception.h"

extern void initWinSock();
extern SOCKET mksock(int type);
extern void bindSock(SOCKET sock);
extern sockaddr_in remote_addr(char *serverip,u_short port);