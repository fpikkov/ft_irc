#pragma once

#include <iostream>
#include "Logger.hpp"

namespace irc
{
	// Minor memory optimization
	constexpr const bool const MEMORY_SAVING = true;

	// Customizable server version
	constexpr const char* const SERVER_VERSION = "v1.0";

	// Hostname length limit in the POSIX standard
	constexpr size_t const MAX_HOSTNAME_LENGTH = 64;

	// Default hostname
	constexpr const char* const DEFAULT_HOSTNAME = "localhost";

	// Backlog value for listen. System maximum is 4096
	constexpr int const MAX_CONNECTION_REQUESTS = 128;

	// Buffer size as defined in the IRC protocol
	constexpr size_t const MAX_IRC_MESSAGE_LENGTH = 512;

	// Maximum incomplete message buffer size
	constexpr int const MAX_CLIENT_BUFFER_SIZE = 4096;

	// Logging statuses
	constexpr const char* const LOG_FAIL = "\033[1;31mFAILURE\033[0m";
	constexpr const char* const LOG_SUCCESS = "\033[1;32mSUCCESS\033[0m";
	constexpr const char* const LOG_DEBUG = "\033[1;36mDEBUG\033[0m";

	// Macros
	inline void print( const auto& msg ) { std::cout << msg << '\n'; }
	inline void log_event( const std::string& func, const std::string& status, const std::string& msg )
	{
		Logger::instance().log( func, status, msg );
	}
}


