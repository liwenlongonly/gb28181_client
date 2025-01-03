#include "General_exception2.h"
#include <vector>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif // _WIN32

std::string format_msg(const char* format, ...) {
	std::string _err_msg;
	va_list args;
	va_start(args, format);
#ifdef _WIN32
	_err_msg.resize(_vscprintf(format, args)); // _vscprintf doesn't count terminating '\0'
	vsprintf_s((char*)_err_msg.data(), _err_msg.size() + 1, format, args);
#else
	char* ps = 0;
	int rt = vasprintf(&ps, format, args);
	if (rt >= 0) {
		_err_msg.assign(ps, rt); free(ps);
	}
#endif
	return _err_msg;
}

char g_app_name[256] = { '\0' };
bool _dummy1 = []
{
	char buf[256] = {};
	size_t sz = 256;
	std::string str;
#ifdef WIN32
	DWORD rt = GetModuleFileNameA(NULL, buf, sz);
	if (rt > 0) {
		str = buf;
		str = str.substr(str.rfind("\\") + 1);
	}
#else
	int rt = readlink("/proc/self/exe", buf, sz);
	if (rt >= 0 && rt < (int)sz)
	{
		str = buf;
		str = str.substr(str.rfind("/") + 1);
	}
#endif
	strcpy(g_app_name, str.data());
	return true;
}();

namespace {

	struct StackInfo
	{
		std::string entryName;
		std::string lineFileName;
		int lineNumber;
		uintptr_t addr;
		StackInfo() :lineNumber(-1),addr(0) {}
	};
}
#ifdef _WIN32
#include <windows.h>
#undef UNICODE
#include <DbgHelp.h>
#include <TlHelp32.h>
#ifdef _MSC_VER
#pragma comment(lib,"dbghelp.lib")
#endif

namespace {

const int STACKWALK_MAX_NAMELEN = 1024;
const int c_MaxRecursionCount = 512;

const HANDLE g_hProcess = [] {
	HANDLE hProcess = GetCurrentProcess();
	if (!SymInitialize(hProcess, NULL, FALSE)) {
		return (HANDLE)NULL;
	}
	DWORD symOptions = SymGetOptions();
	symOptions |= SYMOPT_LOAD_LINES;
	symOptions |= SYMOPT_FAIL_CRITICAL_ERRORS;
	symOptions = SymSetOptions(symOptions);

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	if (hSnap == (HANDLE)-1) {
		return (HANDLE)NULL;
	}
	MODULEENTRY32 me;
	me.dwSize = sizeof(me);
	BOOL haveNext = Module32First(hSnap, &me);
	while (haveNext)
	{
		SymLoadModule64(hProcess, NULL, me.szExePath, me.szModule, (DWORD64)me.modBaseAddr,
			me.modBaseSize);
		haveNext = Module32Next(hSnap, &me); 
	}

	return hProcess;
}(); 

BOOL __stdcall _myReadProcMem(HANDLE  hProcess,
	DWORD64 qwBaseAddress,
	PVOID   lpBuffer,
	DWORD   nSize,
	LPDWORD lpNumberOfBytesRead)
{
	
	SIZE_T st;
	BOOL   bRet = ReadProcessMemory(hProcess, (LPVOID)qwBaseAddress, lpBuffer, nSize, &st);
	*lpNumberOfBytesRead = (DWORD)st;
	return bRet;
}


void get_curr_stacks(std::vector<StackInfo>& vsi)
{
	HANDLE  hThread = GetCurrentThread();
	CONTEXT c = {};
	c.ContextFlags = CONTEXT_FULL;
	RtlCaptureContext(&c);

	// init STACKFRAME for first call
	STACKFRAME64 s; // in/out stackframe
	memset(&s, 0, sizeof(s));
	DWORD imageType;
#ifdef _M_IX86
	// normally, call ImageNtHeader() and use machine info from PE header
	imageType = IMAGE_FILE_MACHINE_I386;
	s.AddrPC.Offset = c.Eip;
	s.AddrPC.Mode = AddrModeFlat;
	s.AddrFrame.Offset = c.Ebp;
	s.AddrFrame.Mode = AddrModeFlat;
	s.AddrStack.Offset = c.Esp;
	s.AddrStack.Mode = AddrModeFlat;
	const int FrameSkip = 2;
#elif _M_X64
	imageType = IMAGE_FILE_MACHINE_AMD64;
	s.AddrPC.Offset = c.Rip;
	s.AddrPC.Mode = AddrModeFlat;
	s.AddrFrame.Offset = c.Rsp;
	s.AddrFrame.Mode = AddrModeFlat;
	s.AddrStack.Offset = c.Rsp;
	s.AddrStack.Mode = AddrModeFlat;
	const int FrameSkip = 3;
#elif _M_IA64
	imageType = IMAGE_FILE_MACHINE_IA64;
	s.AddrPC.Offset = c.StIIP;
	s.AddrPC.Mode = AddrModeFlat;
	s.AddrFrame.Offset = c.IntSp;
	s.AddrFrame.Mode = AddrModeFlat;
	s.AddrBStore.Offset = c.RsBSP;
	s.AddrBStore.Mode = AddrModeFlat;
	s.AddrStack.Offset = c.IntSp;
	s.AddrStack.Mode = AddrModeFlat;
	const int FrameSkip = 3;
#else
#error "Platform not supported!"
#endif

	char bSym[sizeof(IMAGEHLP_SYMBOL64) + STACKWALK_MAX_NAMELEN] = {};
	IMAGEHLP_SYMBOL64* pSym = (IMAGEHLP_SYMBOL64*)&bSym;
	pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
	pSym->MaxNameLength = STACKWALK_MAX_NAMELEN;

	IMAGEHLP_LINE64 Line = {};
	Line.SizeOfStruct = sizeof(Line);

	vsi.clear();
	int curRecursionCount = 0;
	for (int frameNum = 0;; ++frameNum) {
		StackInfo si;
		
		if (!StackWalk64(imageType, g_hProcess, hThread, &s, &c, _myReadProcMem,
			SymFunctionTableAccess64, SymGetModuleBase64, NULL))
		{
			HRESULT hr = GetLastError();
			if (hr != 0) {
				si.entryName = format_msg("\t'StackWalk64' failed. GetLastError=%d @Addr=%p", hr, (void*)s.AddrPC.Offset);
				vsi.push_back(si);
			}
			break;
		}
		if (s.AddrPC.Offset == 0) {
			break;//END of StackWalk.
		}

		if (frameNum<FrameSkip) {
			continue;	//skip the first stacks of "get_curr_stacktrace" and "GeneralException2.Constructor"
		}

		if (s.AddrPC.Offset == s.AddrReturn.Offset) {
			if (curRecursionCount++ > c_MaxRecursionCount) {
				si.entryName = format_msg("\tEndless-Callstack! @Addr=%p", (void*)s.AddrPC.Offset);
				vsi.push_back(si);
				break;
			}
		}
		else {
			curRecursionCount = 0;
		}

		// we seem to have a valid PC
		// show procedure info (SymGetSymFromAddr64())
		DWORD64 offsetFromSmybol = 0;
		std::string &entryName = si.entryName, &lineFileName = si.lineFileName;
		if (SymGetSymFromAddr64(g_hProcess, s.AddrPC.Offset, &(offsetFromSmybol),
			pSym) != FALSE)
		{
			char undName[STACKWALK_MAX_NAMELEN + 1] = {};
			UnDecorateSymbolName(pSym->Name, undName, STACKWALK_MAX_NAMELEN, UNDNAME_COMPLETE);
			entryName = undName[0] == 0 ? pSym->Name : undName;
		}

		DWORD offsetFromLine = 0;
		if (SymGetLineFromAddr64(g_hProcess, s.AddrPC.Offset, &(offsetFromLine),
			&Line) != FALSE)
		{
			si.lineNumber = Line.LineNumber;
			lineFileName = Line.FileName;
		}
		if (si.lineNumber == -1 && !entryName.empty()) {
			break;
		}
		//stacktrace_msg.append(fmt::format("\t{} ({}): {}\n", lineFileName, lineNumber, entryName));
		si.addr = s.AddrPC.Offset;
		vsi.push_back(si);
	}
}

static std::string get_curr_stacktrace(std::string& file_name) {
	std::vector<StackInfo> vsi;
	get_curr_stacks(vsi);
	std::string stacktrace_msg;
	for (auto& s : vsi) 
	{
		if (s.lineNumber > 0) {
			stacktrace_msg.append(format_msg("\t%s (%d): %s\n", s.lineFileName.data(), s.lineNumber, s.entryName.data()));
		}
		else {
			stacktrace_msg.append(format_msg("\t%s [%p]\n", s.lineFileName.data(), (void*)s.addr));
		}
	}

	if (vsi.size() > 0)
	{
		file_name = vsi[0].lineFileName;
#ifdef _WIN32
		size_t pos = file_name.rfind("\\");
#else
		size_t pos = file_name.rfind("/");
#endif
		file_name = file_name.substr(pos + 1);

		file_name += ":" + std::to_string(vsi[0].lineNumber);
	}

	return stacktrace_msg;
}

}

#endif //_WIN32
#ifdef __linux
#include <execinfo.h>
#include <cxxabi.h>

namespace {

const int c_TraceDepth=300;



std::string get_curr_stacktrace(std::string& file_name) 
{
	void* traceBuf[c_TraceDepth] = {};
	int tsize = backtrace(traceBuf, c_TraceDepth);
	char** strings = backtrace_symbols(traceBuf, tsize);
	
	if(strings == NULL) {
		return "";		
	}
	std::string stacktrace_msg;
	for(int i=0;i<tsize;i++) 
	{
		char* line=strings[i];
		stacktrace_msg.append("\t");
		stacktrace_msg.append(line);
	}

	if (tsize > 0)
	{
		file_name = strings[0];
	}

	free(strings);
	return stacktrace_msg;
}

}
#endif //__linux
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


GeneralException2::GeneralException2(int errc) 
	: _err_code(errc)
{
	/*std::string file_name;
	_stack_trace = get_curr_stacktrace(file_name);

	_file_name = file_name.empty() ? "" : ("[" + file_name + "]");
	_err_msg = format_msg("Exception: [%s]%s-[%d]", g_app_name, _file_name.data(), errc);*/
	_err_msg = format_msg("Exception: [%s]-[%d]", g_app_name, errc);
}

GeneralException2::GeneralException2(int errc, const char* errmsg) 
	: _err_code(errc)
{
	/*std::string file_name;
	_stack_trace = get_curr_stacktrace(file_name);

	_file_name = file_name.empty() ? "" : ("[" + file_name + "]");
	
	_err_msg = format_msg("Exception: [%s]%s-[%d],%s", g_app_name, _file_name.data(), errc, errmsg);*/
	_err_msg = format_msg("Exception: [%s]-[%d],%s", g_app_name, errc, errmsg);
}

GeneralException2::GeneralException2(int errc, const ::std::string& errs) 
	: _err_code(errc)
	, _err_msg(errs)
{
	/*std::string file_name;
	_stack_trace = get_curr_stacktrace(file_name);

	_file_name = file_name.empty() ? "" : ("[" + file_name + "]");

	_err_msg = format_msg("Exception: [%s]%s-[%d],%s", g_app_name, _file_name.data(), errc, errs.data());*/
	_err_msg = format_msg("Exception: [%s]-[%d],%s", g_app_name, errc, errs.data());
}

GeneralException2::GeneralException2(int errc, const char* errmsg, int suberrc, const char* suberrmsg)
	: _err_code(errc)
{
	/*std::string file_name;
	_stack_trace = get_curr_stacktrace(file_name);

	_file_name = file_name.empty() ? "" : ("[" + file_name + "]");
	_err_msg = format_msg("Exception: [%s]%s-[%d],%s---cause of:[%d],%s", g_app_name, _file_name.data(), _err_code, errmsg, suberrc, suberrmsg);*/
	_err_msg = format_msg("Exception: [%s]-[%d],%s---cause of:[%d],%s", g_app_name, _err_code, errmsg, suberrc, suberrmsg);
}

GeneralException2::GeneralException2(int errc, const std::string& errs, int suberrc, const std::string& suberrs) 
	: _err_code(errc)
{
	/*std::string file_name;
	_stack_trace = get_curr_stacktrace(file_name);

	_file_name = file_name.empty() ? "" : ("[" + file_name + "]");
	_err_msg = format_msg("Exception: [%s]%s-[%d],%s---cause of:[%d],%s", g_app_name, _file_name.data(), _err_code, errs.data(), suberrc, suberrs.data());*/
	_err_msg = format_msg("Exception: [%s]-[%d],%s---cause of:[%d],%s", g_app_name, _err_code, errs.data(), suberrc, suberrs.data());
}

GeneralException2& GeneralException2::format_errmsg(const char* format, ...) {
	va_list args;
	va_start(args, format);
	std::string errdata;
#ifdef _WIN32
	errdata.resize(_vscprintf(format, args)); // _vscprintf doesn't count terminating '\0'
	vsprintf_s((char*)errdata.data(), errdata.size() + 1, format, args);
#else
	char* ps = 0;
	int rt = vasprintf(&ps, format, args);
	if (rt >= 0) {
		errdata.assign(ps, rt); free(ps);
	}
#endif

	_err_msg = format_msg("Exception: [%s]%s-[%d],%s", g_app_name, _file_name.data(), _err_code, errdata.data());
	return *this;
}
