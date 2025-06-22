#ifndef __SEHCATHER_H__
#define __SEHCATHER_H__
#ifdef _WIN32
#pragma once
#endif

#include "common.hpp"
#include "misc/httptransfer.hpp"
#ifdef _WIN32
#include <imagehlp.h>
#else
#include <sys/signal.h>
#endif

#include <SDL3/SDL.h>

namespace PopLib
{

class AppBase;

#ifdef _WIN32
typedef BOOL (__stdcall * SYMINITIALIZEPROC)(HANDLE, LPSTR, BOOL);

typedef DWORD (__stdcall *SYMSETOPTIONSPROC)(DWORD);

typedef BOOL (__stdcall *SYMCLEANUPPROC)(HANDLE);

typedef LPCSTR (__stdcall *UNDECORATESYMBOLNAMEPROC)(LPCSTR, LPSTR, DWORD, DWORD);

typedef BOOL (__stdcall * STACKWALKPROC)
           ( DWORD, HANDLE, HANDLE, LPSTACKFRAME, LPVOID,
            PREAD_PROCESS_MEMORY_ROUTINE,PFUNCTION_TABLE_ACCESS_ROUTINE,
            PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE);

typedef LPVOID (__stdcall *SYMFUNCTIONTABLEACCESSPROC)(HANDLE, DWORD);

typedef DWORD (__stdcall *SYMGETMODULEBASEPROC)(HANDLE, DWORD);

typedef BOOL (__stdcall *SYMGETSYMFROMADDRPROC)(HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL);


#ifdef _WIN64
	#define SYMFUNCTIONTABLEACCESSPROC PFUNCTION_TABLE_ACCESS_ROUTINE64
	#define SYMGETMODULEBASEPROC PGET_MODULE_BASE_ROUTINE64
#endif
#endif

#ifndef _WIN32
using LPEXCEPTION_POINTERS = void*; // dummy.
#endif

class SEHCatcher 
{
public:	
	static AppBase*		    mApp;
	static bool				mHasDemoFile;
	static bool				mDebugError;
	static std::string		mErrorTitle;
	static std::string		mErrorText;
	static std::string		mUserText;
	static std::string		mUploadFileName;
	static std::wstring		mCrashMessage;
	static std::string		mIssueWebsite;
	static std::wstring		mSubmitErrorMessage;
	static HTTPTransfer		mSubmitReportTransfer;
	static bool				mExiting;
	static bool				mShowUI;
	static bool				mAllowSubmit;
	#ifdef _WIN32
	static HMODULE			mImageHelpLib;
	static SYMINITIALIZEPROC mSymInitialize;
	static SYMSETOPTIONSPROC mSymSetOptions;
	static UNDECORATESYMBOLNAMEPROC mUnDecorateSymbolName;
	static SYMCLEANUPPROC	mSymCleanup;
	static STACKWALKPROC	mStackWalk;
	static SYMFUNCTIONTABLEACCESSPROC mSymFunctionTableAccess;
	static SYMGETMODULEBASEPROC mSymGetModuleBase;
	static SYMGETSYMFROMADDRPROC mSymGetSymFromAddr;
	#endif

#ifdef _WIN32
protected:
	static LPTOP_LEVEL_EXCEPTION_FILTER mPreviousFilter;
#endif

public:
	static void				SubmitReportThread(void *theArg);

	static void SEHWindowProc();
#ifdef _WIN32
	static long __stdcall	UnhandledExceptionFilter(LPEXCEPTION_POINTERS lpExceptPtr);	
	static void				DoHandleDebugEvent(LPEXCEPTION_POINTERS lpEP);
	static std::string		IntelWalk(PCONTEXT theContext, int theSkipCount);
	static std::string		ImageHelpWalk(PCONTEXT theContext, int theSkipCount);
	static bool				LoadImageHelp();
	static void				UnloadImageHelp();
#else
	static long UnhandledExceptionFilter(LPEXCEPTION_POINTERS) { return 0; }
	static void DoHandleDebugEvent(LPEXCEPTION_POINTERS) {}

	static void signalHandler(int sig, siginfo_t* info, void* ucontext);
#endif

	static bool				GetLogicalAddress(void* addr, char* szModule, DWORD len, DWORD& section, DWORD& offset);
	static std::string		GetFilename(const std::string& thePath);
	static void				WriteToFile(const std::string& theErrorText);
	static void				ShowErrorDialog(const std::string& theErrorTitle, const std::string& theErrorText);	

	static std::string		GetSysInfo();
	static void				GetSymbolsFromMapFile(std::string &theDebugDump);

public:
	SEHCatcher();
	~SEHCatcher() noexcept;
};

extern SEHCatcher gSEHCatcher;

}

#endif