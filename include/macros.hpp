#pragma once

#include <iostream>
#include "Logger.hpp"

#define PRINT( msg ) std::cout << msg << '\n';
#define LOG_EVENT( func, status, msg ) \
	Logger::instance().log( std::string(func), status, msg )
