#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <netinet/in.h>
#include <zconf.h>
#include <arpa/inet.h>
#include <bits/unique_ptr.h>
#include <memory>
#include "../Exception.h"
#include "Proto.pb.h"
#include "Global.h"
#include <list>
using namespace std;

typedef list<Proto::UserListNode> UserList;
typedef Proto::UserListNode Node;
typedef Proto::Message Message;
typedef Proto::P2PMessage P2PMessage;

UserList ClientList;

int mksock(int type)
{
    int sock = socket(AF_INET,type,0);
    if(sock < 0)
    {
        perror("sock");
        exit(-1);
    }
    return sock;
}

Node getUser(const char* userName)
{
    for(auto& user : ClientList)
    {
        if(strcmp((user.username().c_str()),userName) == 0)
            return user;
    }
    throw Exception("not find this user");
}

int testNATProp()
{
    try
    {
        int primaryUDP = mksock(SOCK_DGRAM);

        sockaddr_in local;
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        local.sin_port = htons(SERVER_PORT);

        int ret = bind(primaryUDP,(sockaddr*)&local,sizeof(sockaddr));
        if(ret<0)
        {
            throw Exception("bind error");
        }

        sockaddr_in sender;
        char recvbuf;
        bzero(&recvbuf,0);

        while (true)
        {
            unsigned int dwSender = sizeof(sender);

            ret = recvfrom(primaryUDP,(char*)&recvbuf,sizeof(char),0,(sockaddr*)&sender,&dwSender);
            if(ret<0)
            {
                std::cout<<"recv error"<<std::endl;
                continue;
            }

            printf("Receive message from: %s:%ld\n",inet_ntoa(sender.sin_addr),ntohs(sender.sin_port));
        }

    }
    catch (Exception& e)
    {
        std::cout<<e.GetMessage()<<std::endl;
        exit(-1);
    }
}

int main(int argc,char* argv[])
{
    try {
        int primaryUDP = mksock(SOCK_DGRAM);

        sockaddr_in local;
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        local.sin_port = htons(SERVER_PORT);

        ssize_t ret = bind(primaryUDP,(sockaddr*)&local,sizeof(sockaddr));
        if(ret<0)
        {
            throw Exception("bind error");
        }

        sockaddr_in sender;
        Message recvbuf;
        bzero(&recvbuf,0);

        while (true)
        {
            unsigned int dwSender = sizeof(sender);
            char inBuf[sizeof(Message)];
            ret = recvfrom(primaryUDP,inBuf,sizeof(Message),0,(sockaddr*)&sender,&dwSender);
            string inData = inBuf;
            recvbuf.ParseFromString(inData);

            if(ret<0)
            {
                std::cout<<"recv error"<<std::endl;
                continue;
            }
            else
            {
                int messageType = recvbuf.imessagetype();
                switch (messageType)
                {
                    case LOGIN:
                    {
                        std::shared_ptr<Node> currentUser = std::make_shared<Node>();
                        currentUser.get()->set_username(recvbuf.loginmember().username());
                        currentUser.get()->set_ip(ntohl(sender.sin_addr.s_addr));
                        currentUser.get()->set_port(ntohs(sender.sin_port));

                        std::cout<<recvbuf.loginmember().username()<<std::endl;

                        bool bFound = false;
                        for(auto& user : ClientList)
                        {
                            if(user.username() == recvbuf.loginmember().username())
                            {
                                bFound = true;
                                break;
                            }

                        }

                        if(!bFound)
                        {
                            printf("has a user login: %s<->%s:%d\n",
                                   recvbuf.loginmember().username().c_str(),
                                   inet_ntoa(sender.sin_addr),
                                   ntohs(sender.sin_port)
                            );
                            ClientList.emplace_back(*currentUser.get());
                        }

                        int nodeCount = ClientList.size();
                        sendto(primaryUDP,(const char*)&nodeCount,sizeof(int),0,(const sockaddr*)&sender,sizeof(sender));


                        for(const auto& user : ClientList)
                        {
                            string data;
                            user.SerializeToString(&data);
                            sendto(primaryUDP,data.c_str(),data.length(),0,(const sockaddr*)&sender,sizeof(sender));
                            std::cout<<user.port()<<std::endl;
                        }


                        printf("send user list information to: %s <-> %s:%d\n",
                               recvbuf.loginmember().username().c_str(),
                               inet_ntoa(sender.sin_addr),
                               ntohs(sender.sin_port)
                        );

                        break;
                    }
                    case LOGOUT:
                    {
                        printf("has a user logout : %s <-> %s:%d\n", recvbuf.logoutmember().username().c_str(), inet_ntoa( sender.sin_addr ), ntohs(sender.sin_port) );
                       for(auto iter = ClientList.begin();iter!=ClientList.end();iter++)
                       {
                           if( iter->username()== recvbuf.logoutmember().username())
                           {
                               ClientList.erase(iter++);
                               break;
                           }
                       }
                        break;
                    }
                    case P2PTRANS:
                    {
                        printf("%s:%d wants to p2p %s\n",inet_ntoa(sender.sin_addr), ntohs(sender.sin_port), recvbuf.translatemessage().username() );
                        Node node = getUser(recvbuf.translatemessage().username().c_str());
                        sockaddr_in remote;
                        remote.sin_family=AF_INET;
                        remote.sin_port= htons(node.port());
                        remote.sin_addr.s_addr = htonl(node.ip());

                        in_addr tmp;
                        tmp.s_addr = htonl(node.ip());

                        P2PMessage transMessage;
                        transMessage.set_imessagetype(P2PSOMEONEWANTTOCALLYOU);
                        transMessage.set_istringlen(ntohl(sender.sin_addr.s_addr));
                        transMessage.set_port(ntohs(sender.sin_port));

                        sendto(primaryUDP,(const char*)&transMessage, sizeof(transMessage), 0, (const sockaddr *)&remote, sizeof(remote));
                        printf( "tell %s <-> %s:%d to send p2ptrans message to: %s:%d\n",
                                recvbuf.translatemessage().username().c_str(), inet_ntoa(remote.sin_addr), node.port(), inet_ntoa(sender.sin_addr), ntohs(sender.sin_port) );

                        break;
                    }
                    case GETALLUSER:
                    {

                        int command = GETALLUSER;
                        Message sendbuf;
                        sendbuf.set_imessagetype(command);
                        string data;
                        sendbuf.SerializeToString(&data);

                        sendto(primaryUDP, data.c_str(),data.length(), 0, (const sockaddr*)&sender, sizeof(sender));

                        int nodecount = (int)ClientList.size();
                        sendto(primaryUDP, (const char*)&nodecount, sizeof(int), 0, (const sockaddr*)&sender, sizeof(sender));

                        for(UserList::iterator UserIterator=ClientList.begin();
                            UserIterator!=ClientList.end();
                            ++UserIterator)
                        {
                            data.clear();
                            (*UserIterator).SerializeToString(&data);
                            sendto(primaryUDP,data.c_str(), data.length(), 0, (const sockaddr*)&sender, sizeof(sender));
                        }

                        printf("send user list information to: %s <-> %s:%d\n", recvbuf.loginmember().username().c_str(), inet_ntoa( sender.sin_addr ), ntohs(sender.sin_port) );

                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }
    catch(Exception &e)
    {
        printf(e.GetMessage());
        return 1;
    }

    return 0;

}
































