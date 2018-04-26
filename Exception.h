//
// Created by chain on 18-4-3.
//

#ifndef P2P_EXCEPTION_H
#define P2P_EXCEPTION_H


#define EXCEPTION_MESSAGE_MAXLEN 256
#include <cstring>

class Exception
{
private:
    char m_ExceptionMessage[EXCEPTION_MESSAGE_MAXLEN];
public:
    Exception(char *msg):m_ExceptionMessage("")
    {
        strncpy(m_ExceptionMessage, msg, EXCEPTION_MESSAGE_MAXLEN);
    }

    char *GetMessage()
    {
        return m_ExceptionMessage;
    }
};


#endif //P2P_EXCEPTION_H
