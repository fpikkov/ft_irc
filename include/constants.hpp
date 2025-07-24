#pragma once

#include <iostream>
#include "Logger.hpp"

namespace irc
{
	// Available channel modes
	constexpr const char* const CHANNEL_MODES = "i,t,k,o,l";

	// Channel limit (CHANLIMIT)
	constexpr const size_t MAX_CHANNELS = 20;

	// Channel character limit (CHANNELLEN)
	constexpr const int MAX_CHANNEL_LENGTH = 16;

	// Nickname character limit (MAXNICKLEN & NICKLEN)
	constexpr const int MAX_NICKNAME_LENGTH = 16;

	// Username character limit (MAXUSERLEN & USERLEN)
	constexpr const int MAX_USERNAME_LENGTH = 16;

	// Topic character limit (TOPICLEN)
	constexpr const int MAX_TOPIC_LENGTH = 64;

	// Server info used in RPL_WHOISSERVER
	constexpr const char* const SERVER_INFO = "Helsinki, FI";

	// Password requirement
	constexpr const bool REQUIRE_PASSWORD = false;

	// Should broadcasts send the message to the originator
	constexpr const bool BROADCAST_TO_ORIGIN = false;


	// Should the server notify user on hostname lookup
	constexpr const bool ANNOUNCE_CLIENT_LOOKUP = true;

	// Server notice on fetching hostname
	constexpr const char* const CLIENT_HOSTNAME_MESSAGE = "Looking up your hostname...";

	// Server notice on hostname lookup failure
	constexpr const char* const CLIENT_HOSTNAME_FAILURE_MESSAGE = "Couldn't look up your hostname";

	// Server notice on hostname lookup success
	constexpr const char* const CLIENT_HOSTNAME_SUCCESS_MESSAGE = "Hostname retrieved";

	// Reveal real hostname
	constexpr const bool REVEAL_HOSTNAME = true;

	// Minor memory optimization
	constexpr const bool MEMORY_SAVING = false;

	// Additional debug messaging
	constexpr const bool EXTENDED_DEBUG_LOGGING = false;

	// Customizable server version
	constexpr const char* const SERVER_VERSION = "v1.0";

	// Hostname length limit in the POSIX standard
	constexpr const size_t MAX_HOSTNAME_LENGTH = 64;

	// Default hostname
	constexpr const char* const DEFAULT_HOSTNAME = "localhost";

	// Backlog value for listen. System maximum is 4096
	constexpr const int MAX_CONNECTION_REQUESTS = 128;

	// Buffer size as defined in the IRC protocol
	constexpr const size_t MAX_IRC_MESSAGE_LENGTH = 512;

	// Maximum incomplete message buffer size
	constexpr const int MAX_CLIENT_BUFFER_SIZE = 4096;

	// Enable support for the CAP command
	constexpr const bool ENABLE_CAP_SUPPORT = false;

	// Logging statuses
	constexpr const char* const LOG_FAIL	= "\033[1;31mFAILURE\033[0m";
	constexpr const char* const LOG_SUCCESS	= "\033[1;32mSUCCESS\033[0m";
	constexpr const char* const LOG_INFO	= "\033[1;33m INFO  \033[0m";
	constexpr const char* const LOG_DEBUG	= "\033[1;36m DEBUG \033[0m";

	// Macros
	inline void print( const auto& msg ) { std::cout << msg << '\n'; }
	inline void log_event( const std::string& func, const std::string& status, const std::string& msg )
	{
		Logger::instance().log( func, status, msg );
	}
}


