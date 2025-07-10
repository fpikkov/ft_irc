#pragma once

#include <iostream>
#include "Logger.hpp"

namespace irc
{
	// Available channel modes
	constexpr const char* const CHANNEL_MODES = "i,t,k,o,l";

	// Channel limit (CHANLIMIT)
	constexpr const int const MAX_CHANNELS = 20;

	// Channel character limit (CHANNELLEN)
	constexpr const int const MAX_CHANNEL_LENGTH = 16;

	// Nickname character limit (MAXNICKLEN & NICKLEN)
	constexpr const int const MAX_NICKNAME_LENGTH = 16;

	// Topic character limit (TOPICLEN)
	constexpr const int const MAX_TOPIC_LENGTH = 64;

	// Server info used in RPL_WHOISSERVER
	constexpr const char* const SERVER_INFO = "Helsinki, FI";

	// Password requirement
	constexpr const bool const REQUIRE_PASSWORD = false;


	// Should the server notify user on hostname lookup
	constexpr const bool const ANNOUNCE_CLIENT_LOOKUP = true;

	// Server notice on fetching hostname
	constexpr const char* const CLIENT_HOSTNAME_MESSAGE = "Looking up your hostname...";

	// Server notice on hostname lookup failure
	constexpr const char* const CLIENT_HOSTNAME_FAILURE_MESSAGE = "Couldn't look up your hostname";

	// Server notice on hostname lookup success
	constexpr const char* const CLIENT_HOSTNAME_SUCCESS_MESSAGE = "Hostname retrieved";


	// Minor memory optimization
	constexpr const bool MEMORY_SAVING = true;

	// Additional debug messaging
	constexpr const bool EXTENDED_DEBUG_LOGGING = true;

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


