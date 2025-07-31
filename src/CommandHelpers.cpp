#include "CommandHandler.hpp"
#include "constants.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Response.hpp"

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

/**
 * @brief Authenticates the given client if they have provided with all the required fields.
 * Sends welcome messages to the client if they were authenticated successfully.
 *
 * @param client Who to authenticate.
 * @return false if password enforcement is enabled and the user didn't comply with it.
 */
bool	CommandHandler::confirmAuth( Client& client )
{
	if (!client.getNickname().empty() &&
		!client.getUsername().empty() &&
		!client.isAuthenticated())
	{
		if constexpr (irc::REQUIRE_PASSWORD)
		{
			if (!client.getPassValidated())
			{
				Response::sendResponseCode(Response::ERR_PASSWDMISMATCH, client, {});
				return false;
			}
		}
		client.setAuthenticated(true);
		Response::sendWelcome(client);
		irc::log_event("AUTH", irc::LOG_SUCCESS, client.getNickname() + "@" + client.getIpAddress() + " authenticated");
	}
	return true;
}

bool	CommandHandler::isMode( char mode )
{
	return mode == 'i' || mode == 't'  || mode == 'k' || mode == 'o' || mode == 'l';
}

bool	CommandHandler::isSign( char sign )
{
	return sign == '+' || sign == '-';
}

bool	CommandHandler::requiresParam( char mode, bool adding )
{
	return mode == 'o' || ( mode == 'k' && adding ) || ( mode == 'l' && adding );
}

std::string	CommandHandler::inlineParam( const std::string& tokens, size_t& index, std::function<bool(char)> condition )
{
	std::string param;
	while ( index + 1 < tokens.length() && condition(tokens[index + 1]) )
	{
		param += tokens[index + 1];
		++index;
	}
	return param;
}
