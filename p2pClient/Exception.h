#ifndef __HZH_Exception__
#define __HZH_Exception__

#define EXCEPTION_MESSAGE_MAXLEN 256
#include <string>
#include <windows.h>  
#include <strsafe.h>  
#include "Global.h"
class Exception
{
private:
	std::string m_ExceptionMessage;
public:
	Exception(std::string msg)
	{
		errorExit((LPTSTR)msg.c_str());
		m_ExceptionMessage = msg;
	}

	const std::string& GetMessage()
	{
		
		return m_ExceptionMessage;
	}
private:
#ifdef EXIT_GETERRINFO
	void errorExit(LPTSTR lpszFunction)
	{
		// Retrieve the system error message for the last-error code  

		LPVOID lpMsgBuf;
		LPVOID lpDisplayBuf;
		DWORD dw = GetLastError();

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);

		// Display the error message and exit the process  

		lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
			(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
		StringCchPrintf((LPTSTR)lpDisplayBuf,
			LocalSize(lpDisplayBuf) / sizeof(TCHAR),
			TEXT("%s failed with error %d: %s"),
			lpszFunction, dw, lpMsgBuf);
		MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

		LocalFree(lpMsgBuf);
		LocalFree(lpDisplayBuf);
		ExitProcess(dw);
	}
#else
	void errorExit(LPTSTR lpszFunction) {}
#endif // ERRINFO

	
};

#endif