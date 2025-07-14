#include "CommandHandler.hpp"
#include "constants.hpp"

/**
 * @brief Used by handleNick for nickname validation
 */
bool	CommandHandler::isValidNick( const std::string& nick )
{
	if (nick.empty() || nick.length() > 9)
		return false;

	auto isLetter = [](char c) { return std::isalpha(static_cast<unsigned char>(c)); };
	auto isDigit = [](char c) { return std::isdigit(static_cast<unsigned char>(c)); };
	auto isSpecial = [](char c)
	{
		return	c == '[' 	|| c == ']' || c == '\\'
				|| c == '^' || c == '_' || c == '`'
				|| c == '{' || c == '}' || c == '|';
	};

	char firstChar = nick[0];
	if (!isLetter(firstChar) && !isSpecial(firstChar))
		return false;

	for (size_t i = 1; i < nick.length(); ++i)
	{
		char c = nick[i];
		if (!isLetter(c) && !isDigit(c) && !isSpecial(c))
			return false;
	}
	return true;
}

std::string	CommandHandler::toLowerCase( const std::string& s )
{
	std::string result = s;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

bool	CommandHandler::isChannelName( const std::string& name )
{
	return !name.empty() && (name[0] == '#' || name[0] == '&');
}
