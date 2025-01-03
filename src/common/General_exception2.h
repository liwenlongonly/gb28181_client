#ifndef _GENERAL_EXCEPTION2_H_
#define _GENERAL_EXCEPTION2_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <stdarg.h>

std::string format_msg(const char* format, ...);

#ifdef __linux
#include <errno.h>

static inline std::string system_errmsg() {
	char buff[456] = { 0 };
	int ec = errno;
	sprintf(buff, "errno=%d,msg=%s", ec, strerror(ec));
	return buff;
}

#else
#include <WS2tcpip.h>

static inline std::string system_errmsg() {
	char buff[456] = { 0 };
	DWORD dwErr = GetLastError(), off = (DWORD)strlen(buff);
	sprintf_s(buff, "LastError=%u,Msg=", dwErr);
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwErr, 0, buff + off, sizeof(buff) - off, NULL);
	return buff;
}

#endif

class GeneralException2
{	
public:
	GeneralException2(int errc);
	GeneralException2(int errc, const char* errmsg);
	GeneralException2(int errc, const std::string& errs);
	GeneralException2(int errc, const char* errmsg, int suberrc, const char* suberrmsg);
	GeneralException2(int errc, const std::string& errs, int suberrc, const std::string& suberrs);

	virtual ~GeneralException2() {}

	const std::string& stack_trace() const {
		return _stack_trace;
	}

	int err_code() const {
		return _err_code;
	}

	const char* err_str() const {
		return _err_msg.data();
	}

	const std::string& err_msg() const {
		return _err_msg;
	}

	GeneralException2& format_errmsg(const char* format, ...);

private:
	std::string _err_msg;
	std::string _stack_trace;
	std::string _file_name;
	int _err_code;
};

#define THROWABLE(x) x

static inline THROWABLE(void) Check0Throw(int result, const char* msg = "") 
{
	if (result != 0) 
	{
		throw GeneralException2(result, msg);
	}
}

#endif