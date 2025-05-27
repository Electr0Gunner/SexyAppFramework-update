#ifndef __ERRORHANDLER_HPP__
#define __ERRORHANDLER_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "common.hpp"

namespace PopWork
{
class AppBase;

/**
 * @brief the error handler
 *
 * a replacement for SEHCatcher
 */
class ErrorHandler
{
  private:
	// data

  public:
	/// @brief the appbase
	AppBase *mApp;

  public:
	/// @brief constructor
	/// @param theApp 
	ErrorHandler(AppBase *theApp);
	/// @brief destructor
	virtual ~ErrorHandler();
};

/// @brief app error handler
extern ErrorHandler *gErrorHandler;

} // namespace PopWork

#endif