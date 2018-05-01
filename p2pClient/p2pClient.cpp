#include <windows.h>
#include "Global.h"
#include "Exception.h"
#include "sock.h"
#include "utils.h"
#include <iostream>

using namespace std;
#pragma comment(lib,"ws2_32.lib")
#pragma disable(warning C4996)

UserList ClientList;

#define COMMANDMAXC 256
#define MAXRETRY    5

SOCKET PrimaryUDP;
char UserName[10];
char ServerIP[20];

bool RecvedACK;

Node GetUser(char *username)
{
	int length = ClientList.userlistnode_size();
	for (int i = 0; i < length; i++)
	{
		if (strcmp(ClientList.userlistnode(i).username().c_str(), username) == 0)
			return ClientList.userlistnode(i);
	}
	throw Exception("not find this user");
}


//���ӵ�������
void ConnectToServer(SOCKET sock, char *username, char *serverip)
{
	//Զ�̷������ĵ�ַ�ṹ
	sockaddr_in remote = remote_addr(serverip, SERVER_PORT);
	//remote.sin_addr.S_un.S_addr = inet_addr(serverip);
	//remote.sin_family = AF_INET;
	//remote.sin_port = htons(SERVER_PORT);

	//����һ����½����
	Message sendbuf;
	sendbuf.set_imessagetype(LOGIN);
	sendbuf.mutable_loginmember()->set_username(username);
	sendCommand(sock,remote,sendbuf);
	/*string data;
	sendbuf.SerializeToString(&data);
	sendto(sock, data.c_str(), data.length(), 0, (const sockaddr*)&remote, sizeof(remote));*/

	int usercount;
	int fromlen = sizeof(remote);

	for (;; )
	{
		fd_set readfds;
		fd_set writefds;

		FD_ZERO(&readfds);
		FD_ZERO(&writefds);

		FD_SET(sock, &readfds);
		int maxfd = sock;

		timeval to;
		to.tv_sec = 2;
		to.tv_usec = 0;

		int n = select(maxfd + 1, &readfds, &writefds, NULL, &to);

		if (n > 0)
		{
			if (FD_ISSET(sock, &readfds))
			{
				int iread = recvfrom(sock, (char *)&usercount, sizeof(int), 0, (sockaddr *)&remote, &fromlen);
				if (iread <= 0)
				{
					throw Exception("Login error\n");
				}

				break;
			}
		}
		else if (n < 0)
		{
			throw Exception("Login error\n");
		}

		sendto(sock, (const char*)&sendbuf, sizeof(sendbuf), 0, (const sockaddr*)&remote, sizeof(remote));
	}

	// ��¼������˺󣬽��շ���˷������Ѿ���¼���û�����Ϣ
	cout << "Have " << usercount << " users logined server:" << endl;

	char inBuf[sizeof(UserList)];
	recvfrom(sock, inBuf, sizeof(Node), 0, (sockaddr *)&remote, &fromlen);
	string inData = inBuf;
	ClientList.clear_userlistnode();
	ClientList.ParseFromString(inData);

	int count = ClientList.userlistnode_size();
	for (int i = 0; i < count; i++)
	{
		Node node = ClientList.userlistnode(i);
		cout << "Username:" << node.username() << endl;
		in_addr tmp;
		tmp.S_un.S_addr = htonl(node.ip());
		cout << "UserIP:" << inet_ntoa(tmp) << endl;
		cout << "UserPort:" << node.port() << endl;
		cout << "" << endl;
	}
}

void OutputUsage()
{
	cout << "You can input you command:\n"
		<< "Command Type:\"send\",\"tell\", \"exit\",\"getu\"\n"
		<< "Example : send Username Message\n"
		<< "Example : tell Username ip:port Message\n"
		<< "          exit\n"
		<< "          getu\n"
		<< endl;
}

/* ������Ҫ�ĺ���������һ����Ϣ��ĳ���û�(C)
*���̣�ֱ����ĳ���û�������IP������Ϣ�������ǰû����ϵ��
*      ��ô����Ϣ���޷����ͣ����Ͷ˵ȴ���ʱ��
*      ��ʱ�󣬷��Ͷ˽�����һ��������Ϣ������ˣ�
*      Ҫ�����˷��͸��ͻ�Cһ����������C���������ʹ���Ϣ
*      �������̽��ظ�MAXRETRY��
*/
bool SendMessageTo(char *UserName, char *Msg)
{
	char realmessage[256];
	unsigned int UserIP;
	unsigned short UserPort;
	bool FindUser = false;

	int length = ClientList.userlistnode_size();
	for (int i = 0; i < length; i++)
	{
		if (strcmp((ClientList.userlistnode(i).username().c_str()), UserName) == 0)
		{
			UserIP = ClientList.userlistnode(i).ip();
			UserPort = ClientList.userlistnode(i).port();
			FindUser = true;
		}
	}

	if (!FindUser)
		return false;

	strcpy_s(realmessage, Msg);
	for (int i = 0; i < MAXRETRY; i++)
	{
		RecvedACK = false;

		sockaddr_in remote;
		remote.sin_addr.S_un.S_addr = htonl(UserIP);
		remote.sin_family = AF_INET;
		remote.sin_port = htons(UserPort);
		P2PMessage MessageHead;
		MessageHead.set_imessagetype(P2PMESSAGE);
		MessageHead.set_istringlen((int)strlen(realmessage)+1);
		string data;
		MessageHead.SerializeToString(&data);

		int isend = sendto(PrimaryUDP, data.c_str(), data.length(), 0, (const sockaddr*)&remote, sizeof(remote));

		isend = sendto(PrimaryUDP, (const char *)&realmessage, strlen(realmessage) + 1, 0, (const sockaddr*)&remote, sizeof(remote));

		// �ȴ������߳̽��˱���޸�
		for (int j = 0; j < 10; j++)
		{
			if (RecvedACK)
				return true;
			else
				Sleep(300);
		}

		// û�н��յ�Ŀ�������Ļ�Ӧ����ΪĿ�������Ķ˿�ӳ��û��
		// �򿪣���ô����������Ϣ����������Ҫ����������Ŀ������
		// ��ӳ��˿ڣ�UDP�򶴣�
		sockaddr_in server;
		server.sin_addr.S_un.S_addr = inet_addr(ServerIP);
		server.sin_family = AF_INET;
		server.sin_port = htons(SERVER_PORT);

		Message transMessage;
		transMessage.set_imessagetype(P2PTRANS);
		transMessage.mutable_translatemessage()->set_username(UserName);
		data.clear();
		transMessage.SerializeToString(&data);

		sendto(PrimaryUDP, data.c_str(), data.length(), 0, (const sockaddr*)&server, sizeof(server));
		Sleep(100);// �ȴ��Է��ȷ�����Ϣ��
	}
	return false;
}

bool SendMessageTo2(char *UserName, char *Message, const char *pIP, USHORT nPort)
{
	char realmessage[256];
	unsigned int UserIP = 0L;
	unsigned short UserPort = 0;

	if (pIP != NULL)
	{
		UserIP = ntohl(inet_addr(pIP));
		UserPort = nPort;
	}
	else
	{
		bool FindUser = false;

		int length = ClientList.userlistnode_size();
		for (int i = 0; i < length; i++)
		{
			if (strcmp((ClientList.userlistnode(i).username().c_str()), UserName) == 0)
			{
				UserIP = ClientList.userlistnode(i).ip();
				UserPort = ClientList.userlistnode(i).port();
				FindUser = true;
			}
		}
		if (!FindUser)
			return false;
	}

	strcpy_s(realmessage, Message);

	sockaddr_in remote;
	remote.sin_addr.S_un.S_addr = htonl(UserIP);
	remote.sin_family = AF_INET;
	remote.sin_port = htons(UserPort);
	P2PMessage MessageHead;
	MessageHead.set_imessagetype(P2PMESSAGE);
	MessageHead.set_istringlen((int)strlen(realmessage) + 1);

	printf("Send message, %s:%ld -> %s\n", inet_ntoa(remote.sin_addr), ntohs(remote.sin_port), realmessage);

	for (int i = 0; i < MAXRETRY; i++)
	{
		RecvedACK = false;
		string data;
		MessageHead.SerializeToString(&data);
		int isend = sendto(PrimaryUDP, data.c_str(), data.length(), 0, (const sockaddr*)&remote, sizeof(remote));
		isend = sendto(PrimaryUDP, (const char *)&realmessage, MessageHead.istringlen(), 0, (const sockaddr*)&remote, sizeof(remote));

		// �ȴ������߳̽��˱���޸�
		for (int j = 0; j < 10; j++)
		{
			if (RecvedACK)
				return true;
			else
				Sleep(300);
		}
	}
	return false;
}

// ���������ʱֻ��exit��send����
// ����getu�����ȡ��ǰ�������������û�
void ParseCommand(const char * CommandLine)
{
	if (strlen(CommandLine) < 4)
		return;
	char Command[10];
	strncpy_s(Command, CommandLine, 4);
	Command[4] = '\0';

	if (strcmp(Command, "exit") == 0)
	{
		Message sendbuf;
		sendbuf.set_imessagetype(LOGOUT);
		sendbuf.mutable_logoutmember()->set_username(UserName);
		sockaddr_in server;
		server.sin_addr.S_un.S_addr = inet_addr(ServerIP);
		server.sin_family = AF_INET;
		server.sin_port = htons(SERVER_PORT);

		string outData;
		sendbuf.SerializeToString(&outData);

		sendto(PrimaryUDP, outData.c_str(), outData.length(), 0, (const sockaddr *)&server, sizeof(server));
		shutdown(PrimaryUDP, 2);
		closesocket(PrimaryUDP);
		exit(0);
	}
	else if (strcmp(Command, "send") == 0)
	{
		char sendname[20];
		char message[COMMANDMAXC];
		int i;
		for (i = 5;; i++)
		{
			if (CommandLine[i] != ' ')
				sendname[i - 5] = CommandLine[i];
			else
			{
				sendname[i - 5] = '\0';
				break;
			}
		}
		strcpy_s(message, &(CommandLine[i + 1]));
		if (SendMessageTo(sendname, message))
			printf("Send OK!\n");
		else
			printf("Send Failure!\n");
	}
	else if (strcmp(Command, "tell") == 0)
	{
		char sendname[20];
		char sendto[64] = { 0 };
		char message[COMMANDMAXC];
		int i;
		for (i = 5;; i++)
		{
			if (CommandLine[i] != ' ')
				sendname[i - 5] = CommandLine[i];
			else
			{
				sendname[i - 5] = '\0';
				break;
			}
		}

		i++;
		int nStart = i;
		for (;; i++)
		{
			if (CommandLine[i] != ' ')
				sendto[i - nStart] = CommandLine[i];
			else
			{
				sendto[i - nStart] = '\0';
				break;
			}
		}

		strcpy_s(message, &(CommandLine[i + 1]));

		char szIP[32] = { 0 };
		char *p1 = sendto;
		char *p2 = szIP;
		while (*p1 != ':')
		{
			*p2++ = *p1++;
		}

		p1++;
		USHORT nPort = atoi(p1);

		if (SendMessageTo2(sendname, message, strcmp(szIP, "255.255.255.255") ? szIP : NULL, nPort))
			printf("Send OK!\n");
		else
			printf("Send Failure!\n");
	}
	else if (strcmp(Command, "getu") == 0)
	{
		int command = GETALLUSER;
		sockaddr_in server;
		server.sin_addr.S_un.S_addr = inet_addr(ServerIP);
		server.sin_family = AF_INET;
		server.sin_port = htons(SERVER_PORT);

		Message sendbuf;
		sendbuf.set_imessagetype(GETALLUSER);
		string outData;
		sendbuf.SerializeToString(&outData);

		sendto(PrimaryUDP, outData.c_str(), outData.length(), 0, (const sockaddr *)&server, sizeof(server));
	}
}

// ������Ϣ�߳�
DWORD WINAPI RecvThreadProc(LPVOID lpParameter)
{
	sockaddr_in remote;
	int sinlen = sizeof(remote);
	P2PMessage recvbuf;
	for (;;)
	{
		char inBuf[sizeof(P2PMessage)];
		int iread = recvfrom(PrimaryUDP, inBuf, sizeof(P2PMessage), 0, (sockaddr *)&remote, &sinlen);
		string inData = inBuf;
		recvbuf.ParseFromString(inData);
		//int iread = recvfrom(PrimaryUDP, (char *)&recvbuf, sizeof(recvbuf), 0, (sockaddr *)&remote, &sinlen);
		if (iread <= 0)
		{
			printf("recv error\n");
			continue;
		}
		int type = recvbuf.imessagetype();
		switch (recvbuf.imessagetype())
		{
		case P2PMESSAGE:
		{
			// ���յ�P2P����Ϣ
			char *comemessage = new char[recvbuf.istringlen()];
			int iread1 = recvfrom(PrimaryUDP, comemessage, 256, 0, (sockaddr *)&remote, &sinlen);
			comemessage[iread1 - 1] = '\0';
			if (iread1 <= 0)
				throw Exception("Recv Message Error\n");
			else
			{
				printf("Recv a Message, %s:%ld -> %s\n", inet_ntoa(remote.sin_addr), htons(remote.sin_port), comemessage);

				P2PMessage sendbuf;
				sendbuf.set_imessagetype(P2PMESSAGEACK);
				string data;
				sendbuf.SerializeToString(&data);
				sendto(PrimaryUDP, data.c_str(), data.length(), 0, (const sockaddr*)&remote, sizeof(remote));
				printf("Send a Message Ack to %s:%ld\n", inet_ntoa(remote.sin_addr), htons(remote.sin_port));
			}

			delete[]comemessage;
			break;

		}
		case P2PSOMEONEWANTTOCALLYOU:
		{
			// ���յ��������ָ����IP��ַ��
			printf("Recv p2someonewanttocallyou from %s:%ld\n", inet_ntoa(remote.sin_addr), htons(remote.sin_port));

			sockaddr_in remote;
			remote.sin_addr.S_un.S_addr = htonl(recvbuf.istringlen());
			remote.sin_family = AF_INET;
			remote.sin_port = htons(recvbuf.port());

			// UDP hole punching
			P2PMessage message;
			message.set_imessagetype(P2PTRASH);
			string data;
			message.SerializeToString(&data);
			sendto(PrimaryUDP, data.c_str(), data.length(), 0, (const sockaddr*)&remote, sizeof(remote));

			printf("Send p2ptrash to %s:%ld\n", inet_ntoa(remote.sin_addr), htons(remote.sin_port));

			break;
		}
		case P2PMESSAGEACK:
		{
			// ������Ϣ��Ӧ��
			RecvedACK = true;
			printf("Recv message ack from %s:%ld\n", inet_ntoa(remote.sin_addr), htons(remote.sin_port));

			break;
		}
		case P2PTRASH:
		{
			// �Է����͵Ĵ���Ϣ�����Ե���
			//do nothing ...
			printf("Recv p2ptrash data from %s:%ld\n", inet_ntoa(remote.sin_addr), htons(remote.sin_port));

			break;
		}
		case GETALLUSER:
		{
			int usercount;
			int fromlen = sizeof(remote);
			int iread = recvfrom(PrimaryUDP, (char *)&usercount, sizeof(int), 0, (sockaddr *)&remote, &fromlen);
			if (iread <= 0)
			{
				throw Exception("Login error\n");
			}
			ClientList.clear_userlistnode();
			//ClientList.clear();

			cout << "Have " << usercount << " users logined server:" << endl;

			char inBuf[sizeof(UserList)];
			recvfrom(PrimaryUDP, inBuf, sizeof(Node), 0, (sockaddr *)&remote, &fromlen);
			string inData = inBuf;
			ClientList.clear_userlistnode();
			ClientList.ParseFromString(inData);

			int count = ClientList.userlistnode_size();
			for (int i = 0; i < count; i++)
			{
				Node node = ClientList.userlistnode(i);
				cout << "Username:" << node.username() << endl;
				in_addr tmp;
				tmp.S_un.S_addr = htonl(node.ip());
				cout << "UserIP:" << inet_ntoa(tmp) << endl;
				cout << "UserPort:" << node.port() << endl;
				cout << "" << endl;
			}

			break;
		}
		}
	}
}
//
//int testNATProp()
//{
//	try
//	{
//		initWinSock();
//
//		PrimaryUDP = mksock(SOCK_DGRAM);
//		bindSock(PrimaryUDP);
//
//		char szServerIP1[32] = { 0 };
//		char szServerIP2[32] = { 0 };
//
//		cout << "Please input server1 ip:";
//		cin >> szServerIP1;
//
//		cout << "Please input server2 ip:";
//		cin >> szServerIP2;
//
//		sockaddr_in remote1;
//		remote1.sin_addr.S_un.S_addr = inet_addr(szServerIP1);
//		remote1.sin_family = AF_INET;
//		remote1.sin_port = htons(SERVER_PORT);
//
//		sockaddr_in remote2;
//		remote2.sin_addr.S_un.S_addr = inet_addr(szServerIP2);
//		remote2.sin_family = AF_INET;
//		remote2.sin_port = htons(SERVER_PORT);
//
//		char chData = 'A';
//		int nCount = 0;
//
//		for (;;)
//		{
//			nCount++;
//			printf("send message to: %s:%ld, %ld\n", szServerIP1, SERVER_PORT, nCount);
//
//			sendto(PrimaryUDP, (const char*)&chData, sizeof(char), 0, (const sockaddr*)&remote1, sizeof(remote1));
//
//			if (szServerIP2[0] != 'x')
//			{
//				printf("send message to: %s:%ld, %ld\n", szServerIP2, SERVER_PORT, nCount);
//				sendto(PrimaryUDP, (const char*)&chData, sizeof(char), 0, (const sockaddr*)&remote2, sizeof(remote2));
//			}
//
//			Sleep(5000);
//		}
//	}
//	catch (Exception &e)
//	{
//		std::cout << e.GetMessage() << std::endl;
//		return 1;
//	}
//	return 0;
//}


int main(int argc, char* argv[])
{
	//	testNATProp();
	// 	return 0;

	try
	{
		initWinSock();

		PrimaryUDP = mksock(SOCK_DGRAM);
		bindSock(PrimaryUDP);

		//cout << "Please input server ip:";
		//cin >> ServerIP;
		strcpy_s(ServerIP, "192.168.31.137");

		cout << "Please input your name:";
		cin >> UserName;
		//strcpy_s(UserName, "zhangsan");



		ConnectToServer(PrimaryUDP, UserName, ServerIP);

		HANDLE threadhandle = CreateThread(NULL, 0, RecvThreadProc, NULL, NULL, NULL);

		OutputUsage();

		for (;;)
		{
			char Command[COMMANDMAXC];
			string str;
			getline(std::cin, str);
			ParseCommand(str.c_str());
		}
		CloseHandle(threadhandle);
	}
	catch (Exception &e)
	{
		std::cout << e.GetMessage() << std::endl;
		return 1;
	}

	return 0;
}

