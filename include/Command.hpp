#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <ctype.h>

struct Command
{
	std::string					prefix;
	std::string					command;
	std::vector<std::string>	params;
};

Command msgToCmd(std::string message);
