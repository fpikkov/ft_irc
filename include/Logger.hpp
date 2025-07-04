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

		auto timestamp() -> std::string;

	public:
		static auto	instance() -> Logger&;
		auto		log( const std::string& func, const std::string& status, const std::string& msg) -> void;

};
