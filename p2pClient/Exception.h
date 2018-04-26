#ifndef __HZH_Exception__
#define __HZH_Exception__

#define EXCEPTION_MESSAGE_MAXLEN 256
#include <string>
class Exception
{
private:
	std::string m_ExceptionMessage;
public:
	Exception(std::string msg)
	{
		m_ExceptionMessage = msg;
	}

	const std::string& GetMessage()
	{
		return m_ExceptionMessage;
	}
};



#endif