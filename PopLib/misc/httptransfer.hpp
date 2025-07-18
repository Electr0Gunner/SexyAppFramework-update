#ifndef __HTTPTRANSFER_HPP__
#define __HTTPTRANSFER_HPP__
#ifdef _WIN32
#pragma once
#endif

#define CURL_STATICLIB

#include "common.hpp"
#include "curl/curl.h"
#include <thread>

namespace PopLib
{

class HTTPTransfer
{
  public:
	enum
	{
		RESULT_DONE,
		RESULT_NOT_STARTED,
		RESULT_NOT_COMPLETED,
		RESULT_NOT_FOUND,
		RESULT_HTTP_ERROR,
		RESULT_ABORTED,
		RESULT_SOCKET_ERROR,
		RESULT_INVALID_ADDR,
		RESULT_CONNECT_FAIL,
		RESULT_DISCONNECTED
	};

	int mTransferId;
	int mSocket;
	std::string mSendStr;
	std::string mSpecifiedBaseURL;
	std::string mSpecifiedRelURL;
	std::string mURL;
	std::string mProto;
	std::string mHost;
	int mPort;
	std::string mPath;

	int mContentLength;
	std::string mContent;

	bool mTransferPending;
	bool mThreadRunning;
	bool mExiting;
	bool mAborted;
	int mResult;

  private:
	std::thread mThread;

  protected:
	void PrepareTransfer(const std::string &theURL);
	void StartTransfer();
	void GetHelper(const std::string &theURL);

	void PostHelper(const std::string &theURL, const std::string &theParams);

	void Fail(int theResult);
	void GetThreadProc();
	static void GetThreadProcStub(void *theArg);
	static std::string GetAbsURL(const std::string &theBaseURL, const std::string &theRelURL);

	void UpdateStatus();
	bool SocketWait(bool checkRead, bool checkWrite);

  public:
	HTTPTransfer();
	virtual ~HTTPTransfer();

	void Get(const std::string &theURL);
	void Get(const std::string &theBaseURL, const std::string &theRelURL);

	void Post(const std::string &theURL, const std::string &theParams);
	void Post(const std::string &theBaseURL, const std::string &theRelURL, const std::string &theParams);

	void SendRequestString(const std::string &theHost, const std::string &theSendString);

	void Reset();
	void Abort();
	void WaitFor();
	int GetResultCode();
	std::string GetContent();
};

}; // namespace PopLib

#endif