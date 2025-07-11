/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ahentton <ahentton@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/08 13:17:30 by ahentton          #+#    #+#             */
/*   Updated: 2025/07/10 17:13:57 by ahentton         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CommandHandler.hpp"
#include "Channels.hpp"
#include "Client.hpp"
#include "Response.hpp"
#include "Server.hpp"
#include "Command.hpp"
#include "constants.hpp"

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

void	CommandHandler::handleCommand(Client& client, const Command& cmd)
{
	auto it = _handlers.find(cmd.command);
	if (it != _handlers.end())
		it->second(client, cmd);
	else
		Response::sendResponseCode(Response::ERR_UNKNOWNCOMMAND, client, {{"command", cmd.command}});
}

static	std::string	stringToLower(std::string channelName)
{
	std::string result;
	for (size_t i = 0; i < channelName.length(); i++)
	{
		result += std::tolower(channelName[i]);
	}
	return (result);
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
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "PRIVMSG"}});
		return ;
	}

	std::string	target = cmd.params[0];
	std::string	message = cmd.params[1];

	if ( message.empty() )
	{
		Response::sendResponseCode(Response::ERR_NOTEXTTOSEND, client, {});
		return ;
	}

	if (target[0] == '#' || target[0] == '&')
	{
		target = stringToLower(target); //Channels are stored in lowercase.
		Channel *channel = _server.findChannel(target);
		if (!channel)
		{
			Response::sendResponseCode(Response::ERR_NOSUCHCHANNEL, client, {{"channel", target}});
			return ;
		}
		broadcastPrivmsg(client, *channel, message);
	}
	else
	{
		Client	*recipient = _server.findUser(target);
		if (!recipient)
		{
			Response::sendResponseCode(Response::ERR_NOSUCHNICK, client, {{"nick", target}});
			return ;
		}
		Response::sendResponseCommand("PRIVMSG", client, *recipient, {{"target", recipient->getNickname()}, {"message", message}});
	}
}

/**
 * NOTE: NOTICE is very much like PRIVMSG, but it does not care about errors.
 * No replies are expected from either the server or the recipient.
 *
 * If no message is contained, then nothing will be sent to the target(s).
 */
void	CommandHandler::handleNotice( Client& client, const Command& cmd)
{
	std::string	target = cmd.params[0];
	std::string	message = cmd.params[1];

	if ( message.empty() )
		return ;

	if (target[0] == '#')
	{
		target = stringToLower(target);
		Channel	*channel = _server.findChannel(target);
		if (!channel)
			return ;

		//Send the message to the clients who belong to server.
		const auto& allClients = _server.getClients();

		for ( auto fd : channel->getMembers() )
		{
			if ( fd == client.getFd() && !irc::BROADCAST_TO_ORIGIN ) continue;

			auto memberIt = allClients.find(fd);
			if (memberIt != allClients.end())
			{
				Client& channelMember = const_cast<Client&>(memberIt->second);
				Response::sendResponseCommand("NOTICE", client, channelMember, {{ "message", message }});
			}
		}
	}
	else
	{
		Client *recipient = _server.findUser(target);
		if (!recipient)
			return ;

		Response::sendResponseCommand("NOTICE", client, *recipient, {{ "message", message }});
	}
}

/* CHANNEL COMMANDS*/

void	CommandHandler::handleJoin(Client& client, const Command& cmd)
{
	if (!client.isAuthenticated())
	{
		Response::sendResponseCode(Response::ERR_NOTREGISTERED, client, {});
		return ;
	}
	if (cmd.params[0].empty())
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "JOIN"}});
		return ;
	}
	if (client.getChannels().size() > irc::MAX_CHANNELS)
	{
		Response::sendResponseCode(Response::ERR_TOOMANYCHANNELS, client, {{"channel", cmd.params[0]}});
		return ;
	}

	std::string	target = stringToLower(cmd.params[0]); //Channel names are stored in lowercase..
	std::string	key;
	if (!cmd.params[1].empty())
		key = cmd.params[1];

	Channel* channel = _server.findChannel(target);

	if (!channel)
	{
		_server.addChannel(target);
		channel = _server.findChannel(target);
		if (!key.empty())
			channel->setKey(key);
		channel->addMember(client.getFd());
		channel->addOperator(client.getFd());

		irc::log_event("CHANNEL", irc::LOG_DEBUG, "new channel created");
		broadcastJoin(client, *channel);
		return ;
	}

	const std::string channelName = channel->getName();

	if (channel->isFull())
	{
		Response::sendResponseCode(Response::ERR_CHANNELISFULL, client, {{"channel", channelName}});
		return ;
	}

	if (channel->isInviteOnly())
	{
		if (channel->isInvited(client.getFd()))
		{
			if (channel->addMember(client.getFd()) == true)
			{
				broadcastJoin(client, *channel);
			}
		}
		else
			Response::sendResponseCode(Response::ERR_INVITEONLYCHAN, client, {{"channel", channelName}});
		return ;
	}

	if ( (channel->getKey() && key.empty()) || (channel->getKey() != key) )
	{
		Response::sendResponseCode(Response::ERR_BADCHANNELKEY, client, {{"channel", channelName}});
		return ;
	}
	else
	{
		if (channel->addMember(client.getFd()) == true)
		{
			broadcastJoin(client, *channel);
		}
		else
			return ;
	}
}

void	CommandHandler::handlePart(Client& client, const Command& cmd)
{
	if (!client.isAuthenticated())
	{
		Response::sendResponseCode(Response::ERR_NOTREGISTERED, client, {});
		return ;
	}
	if (cmd.params.size() < 1)
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "PART"}});
		return ;
	}

	std::string	channelName = stringToLower(cmd.params[0]); //Channel names are stored in lowercase
	std::string	optionalMessage = (cmd.params.size() > 1) ? cmd.params[1] : "";

	Channel* channel = _server.findChannel(channelName);

	if (!channel)
	{
		Response::sendResponseCode(Response::ERR_NOSUCHCHANNEL, client, {{"channel", channelName}});
		return ;
	}
	if (!channel->isMember(client.getFd()))
	{
		Response::sendResponseCode(Response::ERR_NOTONCHANNEL, client, {{"channel", channel->getName()}});
		return ;
	}

	// Broadcasts to all members that the user has parted the channel. Empty reason is defaulted to client nicknme.
	const auto& allClients = _server.getClients();
	for ( const auto memberFd : channel->getMembers() )
	{
		if (memberFd == client.getFd() && !irc::BROADCAST_TO_ORIGIN) continue;

		auto memberIt = allClients.find(memberFd);
		if (memberIt != allClients.end())
		{
			Client& channelMember = const_cast<Client&>(memberIt->second);
			Response::sendResponseCommand("PART", client, channelMember, {{"channel", channelName}, { "reason", optionalMessage }});
		}
	}
	channel->removeMember(client.getFd());

	// Remove the channel if no members exist
	if (channel->isEmpty())
	{
		irc::log_event("CHANNEL", irc::LOG_DEBUG, "removed");
		_server.removeChannel(channel->getName());
		return ;
	}
}

void	CommandHandler::handleKick(Client& client, const Command& cmd)
{
	if (!client.isAuthenticated())
	{
		Response::sendResponseCode(Response::ERR_NOTREGISTERED, client, {});
		return ;
	}
	if (cmd.params.size() < 2)
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "KICK"}});
		return ;
	}

	Channel*	channel = _server.findChannel(cmd.params[0]);
	Client*		kickNick = _server.findUser(cmd.params[1]);

	if (!channel)
	{
		Response::sendResponseCode(Response::ERR_NOSUCHCHANNEL, client, {{"channel", cmd.params[0]}});
		return ;
	}
	if (!kickNick)
	{
		Response::sendResponseCode(Response::ERR_NOSUCHNICK, client, {});
		return ;
	}
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
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "PASS"}});
		return ;
	}

	const std::string& providedPassword = cmd.params[0];
	if (providedPassword != _server.getPassword())
	{
		Response::sendResponseCode(Response::ERR_PASSWDMISMATCH, client, {});
		if ( irc::REQUIRE_PASSWORD )
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

	// Truncating username to configurable length (USERLEN)
	if (username.length() > irc::MAX_USERNAME_LENGTH)
		username = username.substr(0, irc::MAX_USERNAME_LENGTH);

	username = "~" + username;

	if (!realname.empty() && realname[0] == ':')
		realname = realname.substr(1);

	client.setUsername(username);
	client.setRealname(realname);
	client.setAuthenticated(true);
}

/* Rest of the commands */

void CommandHandler::handleQuit(Client& client, const Command& cmd)
{
	// Has tto be empty string in case reason is not given.
	std::string quitMessage = "";
	if (!cmd.params.empty())
	{
		quitMessage = cmd.params[0];
		if (!quitMessage.empty() && quitMessage[0] == ':')
			quitMessage = quitMessage.substr(1);
	}
	// In order to be able to send a message we need Client object, not just fd. Called once outside of the loop.
	const auto& allClients = _server.getClients();
	//For every channel the "client" is a member of
	for (const std::string& channelName : client.getChannels())
	{
		// Finding the channel object by name
		Channel* channel = _server.findChannel(const_cast<std::string&>(channelName));
		// If channel does not exist
		if (!channel) continue;
		// For every member in this channel
		for (int memberFd : channel->getMembers())
		{
			if (memberFd == client.getFd() && !irc::BROADCAST_TO_ORIGIN) continue;
			// Find member client object by the fd, because Channel class tracks members as a set of file descriptors (int), not as Client objects.
			auto memberIt = allClients.find(memberFd);
			if (memberIt != allClients.end())
			{
				Client& channelMember = const_cast<Client&>(memberIt->second);
				Response::sendResponseCommand("QUIT", client, channelMember, {{ "reason", quitMessage }});
			}
		}
		channel->removeMember(client.getFd());
		channel->removeOperator(client.getFd());

		if (channel->isEmpty())
		{
			irc::log_event("CHANNEL", irc::LOG_DEBUG, "removed");
			_server.removeChannel(channel->getName());
			return ;
		}
	}

	// Client is set as inactive and disconnection event gets announced to the server
	client.setActive(false);
	Server::setDisconnectEvent(true);
}

void CommandHandler::handlePing(Client& client, const Command& cmd)
{
	if (cmd.params.empty())
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "USER"}}); // The outer { ... } is for the map initializer. The inner { ... } is for each key-value pair.
		return ;
	}
	std::string token = cmd.params[0];

	Response::sendPong( client, token );
}

void CommandHandler::handlePong( [[maybe_unused]] Client& client, [[maybe_unused]] const Command& cmd)
{
	// PONG will check if the message matched the serverside ping sent to client
	// Update client's last active date if implementing timeouts.
	// Server will NOT respond to pongs.
}


// Helper function

/**
 * @brief Broadcast JOIN message to all members of a channel. Also outputs the list of NAMES to the client.
 */
void	CommandHandler::broadcastJoin( Client& client, Channel& channel )
{
	const std::string channelName = channel.getName();
	const auto& allClients = _server.getClients();

	if ( irc::EXTENDED_DEBUG_LOGGING )
		irc::log_event("CHANNEL", irc::LOG_DEBUG, "broadcast evemt");

	for ( const auto memberFd : channel.getMembers() )
	{
		if (memberFd == client.getFd() && !irc::BROADCAST_TO_ORIGIN) continue;

		auto memberIt = allClients.find(memberFd);
		if (memberIt != allClients.end())
		{
			Client& channelMember = const_cast<Client&>(memberIt->second);
			Response::sendResponseCommand("JOIN", client, channelMember, {{"channel", channelName}});
			Response::sendResponseCode(Response::RPL_NAMREPLY, client, {{"symbol", ""}, {"channel", channelName}, {"names", channelMember.getNickname()}});
		}
	}
	Response::sendResponseCode(Response::RPL_ENDOFNAMES, client, {{"channel", channelName}});
}

void	CommandHandler::broadcastPrivmsg( Client& client, Channel& channel, const std::string& message )
{
	const auto& allClients = _server.getClients();

	if ( irc::EXTENDED_DEBUG_LOGGING )
		irc::log_event("CHANNEL", irc::LOG_DEBUG, "broadcast evemt");

	for ( const auto memberFd : channel.getMembers() )
	{
		if (memberFd == client.getFd()) continue;

		auto memberIt = allClients.find(memberFd);
		if (memberIt != allClients.end())
		{
			Client& channelMember = const_cast<Client&>(memberIt->second);
			Response::sendResponseCommand("PRIVMSG", client, channelMember, {{"target", channelMember.getNickname()}, {"message", message}});
		}
	}
}
