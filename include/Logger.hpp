#pragma once

#include "headers.hpp"
#include <ctime>
#include <chrono>
#include <iomanip>
#include <sstream>

class Logger
{
	private:
		Logger();
		Logger( const Logger& )				= delete;
		Logger& operator=( const Logger& )	= delete;


	public:
		static Logger&		instance();
		static std::string	timestamp();
		void				log( const std::string& func, const std::string& status, const std::string& msg);

};
