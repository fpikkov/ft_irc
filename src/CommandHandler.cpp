/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rkhakimu <rkhakimu@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/08 13:17:30 by ahentton          #+#    #+#             */
/*   Updated: 2025/07/09 14:58:39 by rkhakimu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */



#include "CommandHandler.hpp"

CommandHandler::CommandHandler(Server& server) : _server(server)
{
	// Registration commands
	_handlers["PASS"]		= [this](Client& c, const Command& cmd) { handlePass(c, cmd); };
	_handlers["NICK"] 		= [this](Client& c, const Command& cmd) { handleNick(c, cmd); };
	_handlers["USER"] 		= [this](Client& c, const Command& cmd) { handleUser(c, cmd); };

	// Message commands
	_handlers["PRIVMSG"] 	= [this](Client& c, const Command& cmd) { handlePrivmsg(c, cmd); };
	_handlers["NOTICE"] 	= [this](Client& c, const Command& cmd) { handleNotice(c, cmd); };

	// Channel commands
	_handlers["JOIN"]		= [this](Client& c, const Command& cmd) { handleJoin(c, cmd); };
	_handlers["PART"]		= [this](Client& c, const Command& cmd) { handlePart(c, cmd); };
	_handlers["KICK"]		= [this](Client& c, const Command& cmd) { handleKick(c, cmd); };
	_handlers["INVITE"]		= [this](Client& c, const Command& cmd) { handleInvite(c, cmd); };
	_handlers["TOPIC"]		= [this](Client& c, const Command& cmd) { handleTopic(c, cmd); };
	_handlers["MODE"]		= [this](Client& c, const Command& cmd) { handleMode(c, cmd); };

	//Rest of the commands
	_handlers["QUIT"]		= [this](Client& c, const Command& cmd) { handleQuit(c, cmd); };
	_handlers["PING"]		= [this](Client& c, const Command& cmd) { handlePing(c, cmd); };
	_handlers["PONG"]		= [this](Client& c, const Command& cmd) { handlePong(c, cmd); };
}

/*Command handler function, for fast execution of any command.
  This function iterates through _handlers(map of command keys and funcs),
  finds a match, and executes the command internally.
  Server response to client will also be sent internally.
  If no match is found, server immediately responds with unknown command error.*/
  
void    CommandHandler::handleCommand(Client& client, const Command& cmd)
{
	auto it = _handlers.find(cmd.command);
	if (it != _handlers.end())
		it->second(client, cmd);
	else
		Response::sendResponseCode(Response::ERR_UNKNOWNCOMMAND, client, {});
}

static	std::string	stringToLower(std::string channelName)
{
	for (size_t i = 0; i < channelName.length(); i++)
	{
		std::tolower(channelName[i]);
	}
	return (channelName);
}

/* MESSAGE COMMANDS*/

void	CommandHandler::handlePrivmsg(Client& client, const Command& cmd)
{
	if (!client.isAuthenticated())
	{
		Response::sendResponseCode(Response::ERR_NOTREGISTERED, client, {});
		return ;
	}
	else if (cmd.params.size() < 2)
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {});
		return ;
	}

	std::string	target = cmd.params[0];
	std::string	message = cmd.params[1];

	if (target[0] == '#')
	{
		target = stringToLower(target); //Channels are stored in lowercase.
		Channel *channel = _server.findChannel(target);
		if (!channel)
		{
			Response::sendResponseCode(Response::ERR_NOSUCHCHANNEL, client, {});
			return ;
		}
		//Send to all clients who belong in the channel.
	}
	else
	{
		Client	*recipient = _server.findUser(target);
		if (!recipient)
		{
			Response::sendResponseCode(Response::ERR_NOSUCHNICK, client, {});
			return ;
		}
		//Send to the recipient client
	}
}

/* NOTE: NOTICE is very much like PRIVMSG, but it does not care about errors.
   No replies are expected from either the server or the recipient.*/
   
void	CommandHandler::handleNotice(Client& client, const Command& cmd)
{
	std::string	target = cmd.params[0];
	std::string	message = cmd.params[1];

	if (target[0] == '#')
	{
		target = stringToLower(target);
		Channel	*channel = _server.findChannel(target);
		if (!channel)
			return ;
		//Send the message to the clients who belong to server.
	}
	else
	{
		Client *recipient = _server.findUser(target);
		if (!recipient)
			return ;
		//Send the message to the recipient.
	}
}

/* CHANNEL COMMANDS*/

void	CommandHandler::handleJoin(Client&, const Command&)
{
	
}

/* REGISTRATION COMMANDS*/

void	CommandHandler::handlePass(Client& client, const Command& cmd)
{
	if (client.isAuthenticated())
	{
		Response::sendResponseCode(Response::ERR_ALREADYREGISTERED, client, {});
		return ;
	}
	
	if (cmd.params.empty())
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {});
		return ;
	}
	
	const std::string& providedPassword = cmd.params[0];
	if (providedPassword != _server.getPassword())
	{
		Response::sendResponseCode(Response::ERR_PASSWDMISMATCH, client, {});
		if (irc::PASSWORD_REQUIRED)
			return ;
	}
	
	client.setAuthenticated(true);
}


// Helper function for handleNick function

static bool	isValidNick(const std::string& nick)
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

void CommandHandler::handleNick(Client& client, const Command& cmd)
{
	if (cmd.params.empty())
	{
		Response::sendResponseCode(Response::ERR_NONICKNAMEGIVEN, client, {});
		return ;
	}
	
	std::string newNick = cmd.params[0];
	
	if (!isValidNick(newNick))
	{
		Response::sendResponseCode(Response::ERR_ERRONEUSNICKNAME, client, {});
		return ;
	}

	for (const auto& [fd, clientObject] : _server.getClients())
	{
		if (clientObject.getNickname() == newNick)
		{
			Response::sendResponseCode(Response::ERR_NICKNAMEINUSE, client, {});
			return ;
		}
	}
	client.setNickname(newNick);
}

/* 
[ \ ] ^ _ ` { | }
*/
/* USER <username> <mode> <unused> <realname> */
void CommandHandler::handleUser(Client& client, const Command& cmd)
{
	if (client.isAuthenticated())
	{
		Response::sendResponseCode(Response::ERR_ALREADYREGISTERED, client, {});
		return ;
	}
	
	if (cmd.params.size() < 4)
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "USER"}}); // The outer { ... } is for the map initializer. The inner { ... } is for each key-value pair.
		return ;
	}

	std::string username = cmd.params[0];
	std::string realname = cmd.params[3];
	
	if (username.empty())
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "USER"}});
		return ;
	}
	
	// Truncating username to 10 chars (USERLEN)
	const size_t USERLEN = 10;
	if (username.length() > USERLEN)
		username = username.substr(0, USERLEN);
	
	username = "~" + username;

	if (!realname.empty() && realname[0] == ':')
		realname = realname.substr(1);
	
	client.setUsername(username);
	client.setRealname(realname);
	client.setAuthenticated(true);
}

/* Rest of the commands */

