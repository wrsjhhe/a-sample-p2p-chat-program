#pragma once
// ����iMessageType��ֵ
#define LOGIN 1
#define LOGOUT 2
#define P2PTRANS 3
#define GETALLUSER  4

// �������˿�
#define SERVER_PORT 6060

//======================================
// �����Э�����ڿͻ���֮���ͨ��
//======================================
#define P2PMESSAGE 100               // ������Ϣ
#define P2PMESSAGEACK 101            // �յ���Ϣ��Ӧ��
#define P2PSOMEONEWANTTOCALLYOU 102  // ��������ͻ��˷��͵���Ϣ
// ϣ���˿ͻ��˷���һ��UDP�򶴰�
#define P2PTRASH        103          // �ͻ��˷��͵Ĵ򶴰������ն�Ӧ�ú��Դ���Ϣ

