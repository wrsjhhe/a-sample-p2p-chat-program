#include "utils.h"
#include <string>


int sendCommand(SOCKET sock,const sockaddr_in& remote, const Message& sendbuf)
{
	std::string data;
	sendbuf.SerializeToString(&data);
	return sendto(sock, data.c_str(), data.length(), 0, (const sockaddr*)&remote, sizeof(remote));
}