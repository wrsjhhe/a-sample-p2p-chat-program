#pragma once
#include <windows.h>
#include "Proto.pb.h"

// 定义iMessageType的值
#define LOGIN 1
#define LOGOUT 2
#define P2PTRANS 3
#define GETALLUSER  4

// 服务器端口
#define SERVER_PORT 6060

//======================================
// 下面的协议用于客户端之间的通信
//======================================
#define P2PMESSAGE 100               // 发送消息
#define P2PMESSAGEACK 101            // 收到消息的应答
#define P2PSOMEONEWANTTOCALLYOU 102  // 服务器向客户端发送的消息
// 希望此客户端发送一个UDP打洞包
#define P2PTRASH        103          // 客户端发送的打洞包，接收端应该忽略此消息

#define EXIT_GETERRINFO  //弹出错误信息


typedef Proto::UserListNode Node;
typedef Proto::UserList UserList;
typedef Proto::Message Message;
typedef Proto::P2PMessage P2PMessage;