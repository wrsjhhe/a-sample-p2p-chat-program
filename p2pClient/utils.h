#pragma once
#include "Global.h"

extern int sendCommand(SOCKET sock, const sockaddr_in& remote, const Message& sendbuf);