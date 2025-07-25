#include "CommandHandler.hpp"
#include "Channels.hpp"
#include "Client.hpp"
#include "Response.hpp"
#include "Server.hpp"
#include "Command.hpp"
#include "constants.hpp"
#include <algorithm>


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

/**
 * Command handler function, for fast execution of any command.
 * This function iterates through _handlers(map of command keys and funcs),
 * finds a match, and executes the command internally.
 * Server response to client will also be sent internally.
 * If no match is found, server immediately responds with unknown command error.
 */
void	CommandHandler::handleCommand(Client& client, const Command& cmd)
{
	auto it = _handlers.find(cmd.command);
	if (it != _handlers.end())
	{
		if constexpr ( irc::ENABLE_COMMAND_LOGGING )
			irc::log_event("COMMAND", irc::LOG_INFO, cmd.command + " from " + (client.getNickname().empty() ? "*" : client.getNickname()) + "@" + client.getIpAddress());
		it->second(client, cmd);
	}
	else
	{
		if constexpr ( irc::ENABLE_COMMAND_LOGGING )
			irc::log_event("COMMAND", irc::LOG_FAIL, "unknown " + cmd.command + " from " + (client.getNickname().empty() ? "*" : client.getNickname()) + "@" + client.getIpAddress());
		Response::sendResponseCode(Response::ERR_UNKNOWNCOMMAND, client, {{"command", cmd.command}});
	}
}


/* MESSAGE COMMANDS */

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
		target = toLowerCase(target); //Channels are stored in lowercase.
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
	if (!client.isAuthenticated())
	{
		Response::sendResponseCode(Response::ERR_NOTREGISTERED, client, {});
		return ;
	}
	if ( cmd.params.size() < 2)
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "NOTICE"}});
		return ;
	}

	std::string	target	= cmd.params[0];
	std::string	message	= cmd.params[1];

	if ( message.empty() || target.empty() )
		return ;

	if (target[0] == '#')
	{
		target = toLowerCase(target);
		Channel	*channel = _server.findChannel(target);
		if (!channel)
			return ;

		broadcastNotice(client, *channel, message);
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
	if (cmd.params.empty() || cmd.params[0].empty()) // Also check that the vector isn't empty
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "JOIN"}});
		return ;
	}
	if (client.getChannels().size() == irc::MAX_CHANNELS)
	{
		Response::sendResponseCode(Response::ERR_TOOMANYCHANNELS, client, {{"channel", cmd.params[0]}});
		return ;
	}

	std::string	target	= toLowerCase(cmd.params[0]); //Channel names are stored in lowercase..
	std::string	key		= (cmd.params.size() > 1) ? cmd.params[1] : "";

	Channel* channel	= _server.findChannel(target);

	if (!channel) // if channel does not exist, create the channel, add the member to channel and make them an operator.
	{
		_server.addChannel(target);
		channel = _server.findChannel(target);

		if (!channel)
		{
			irc::log_event("CHANNEL", irc::LOG_FAIL, "failed to create: " + target);
			return ;
		}
		irc::log_event("CHANNEL", irc::LOG_INFO, "created: " + target);

		if (!key.empty())
			channel->setKey(key);

		channel->addMember(client.getFd());
		client.joinChannel(channel->getName());
		channel->addOperator(client.getFd());
		broadcastMode(client, *channel, "+o", cmd );
		//TODO: #6 Make sure the user is succesfully made an operator. Irssi does not register this change currently

		irc::log_event("CHANNEL", irc::LOG_INFO, client.getNickname() + "@" + client.getIpAddress() + " joined " + target);
		broadcastJoin(client, *channel);
		return ;
	}

	if (channel->isFull())
	{
		Response::sendResponseCode(Response::ERR_CHANNELISFULL, client, {{"channel", target}});
		return ;
	}

	if (channel->isInviteOnly())
	{
		if (channel->isInvited(client.getFd()))
		{
			if (channel->addMember(client.getFd()) == true)
			{
				client.joinChannel(channel->getName());
				irc::log_event("CHANNEL", irc::LOG_INFO, client.getNickname() + "@" + client.getIpAddress() + " joined " + target);
				broadcastJoin(client, *channel);
			}
		}
		else
			Response::sendResponseCode(Response::ERR_INVITEONLYCHAN, client, {{"channel", target}});
		return ;
	}

	if ( channel->getKey().has_value() )
	{
		if ( key.empty() || (channel->getKey().value() != key) )
		{
			Response::sendResponseCode(Response::ERR_BADCHANNELKEY, client, {{"channel", target}});
			return ;
		}
		else if ( !key.empty() && channel->getKey().value() == key )
		{
			if (channel->addMember(client.getFd()) == true)
			{
				client.joinChannel(channel->getName());
				irc::log_event("CHANNEL", irc::LOG_INFO, client.getNickname() + "@" + client.getIpAddress() + " joined " + target);
				broadcastJoin(client, *channel);
			}
			return ;
		}
	}
	else
	{
		if (channel->addMember(client.getFd()) == true)
		{
			client.joinChannel(channel->getName());
			irc::log_event("CHANNEL", irc::LOG_INFO, client.getNickname() + "@" + client.getIpAddress() + " joined " + target);
			broadcastJoin(client, *channel);
		}
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

	std::string	channelName		= toLowerCase(cmd.params[0]); //Channel names are stored in lowercase
	std::string	optionalMessage	= (cmd.params.size() > 1) ? cmd.params[1] : "";

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

	broadcastPart(client, *channel, optionalMessage);

	irc::log_event("CHANNEL", irc::LOG_INFO, client.getNickname() + "@" + client.getIpAddress() + " left " + channelName);
	channel->removeMember(client.getFd());
	client.leaveChannel(channel->getName());

	// Remove the channel if no members exist after leaving.
	if (channel->isEmpty())
	{
		irc::log_event("CHANNEL", irc::LOG_INFO, "removed: " + channelName);
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
	Client*		target = _server.findUser(cmd.params[1]);
	std::string	message = (cmd.params.size() > 2) ? cmd.params[3] : "";

	if (!channel)
	{
		Response::sendResponseCode(Response::ERR_NOSUCHCHANNEL, client, {{"channel", cmd.params[0]}});
		return ;
	}
	if (!target)
	{
		Response::sendResponseCode(Response::ERR_NOSUCHNICK, client, {});
		return ;
	}
	if (!channel->isOperator(client.getFd()))
	{
		Response::sendResponseCode(Response::ERR_CHANOPRIVSNEEDED, client, {{"channel", channel->getName()}});
		return ;
	}
	if (!channel->isMember(target->getFd()))
	{
		Response::sendResponseCode(Response::ERR_USERNOTINCHANNEL, client, {{"target", target->getNickname()}, \
																			{"channel", channel->getName()}});
		return ;
	}
	if (!channel->isMember(client.getFd()))
	{
		Response::sendResponseCode(Response::ERR_NOTONCHANNEL, client, {{"channel", channel->getName()}});
		return ;
	}
	channel->removeMember(target->getFd());
	target->leaveChannel(channel->getName());
	broadcastKick(client, *channel, message);
}

void	CommandHandler::handleInvite(Client& client, const Command& cmd)
{
	if (!client.isAuthenticated())
	{
		Response::sendResponseCode(Response::ERR_NOTREGISTERED, client, {});
		return ;
	}
	if ( cmd.params.size() < 2 || cmd.params.empty() )
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "INVITE"}});
		return ;
	}

	std::string	channelName = toLowerCase(cmd.params[1]);
	std::string targetName = cmd.params[0];

	Client* target = _server.findUser(targetName);
	Channel* channel = _server.findChannel(channelName);

	if (!target)
	{
		Response::sendResponseCode(Response::ERR_NOSUCHNICK, client, {{"target", targetName}});
		return ;
	}
	if (!channel)
	{
		Response::sendResponseCode(Response::ERR_NOSUCHCHANNEL, client, {{"channel", channelName}});
		return ;
	}
	if (channel->isMember(target->getFd()))
	{
		Response::sendResponseCode(Response::ERR_USERONCHANNEL, client, {{"target", target->getNickname()}});
		return ;
	}
	if (channel->isInviteOnly() && !channel->isOperator(client.getFd()))
	{
		Response::sendResponseCode(Response::ERR_CHANOPRIVSNEEDED, client, {{"channel", channel->getName()}});
		return ;
	}
	channel->invite(target->getFd());
	Response::sendResponseCommand("INVITE", client, *target, {{"target", target->getNickname() }, {"channel", channel->getName()}});
	Response::sendResponseCode(Response::RPL_INVITING, client, {{"channel", channelName}, {"target", targetName}});
}

void	CommandHandler::handleTopic(Client& client, const Command& cmd)
{
	if (!client.isAuthenticated())
	{
		Response::sendResponseCode(Response::ERR_NOTREGISTERED, client, {});
		return ;
	}
	if (cmd.params.size() < 1)
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {});
		return ;
	}

	std::string	channelName = toLowerCase(cmd.params[0]);
	std::string	newTopic = (cmd.params.size() >= 2) ? cmd.params[1] : "";

	Channel* channel = _server.findChannel(channelName);

	if (!channel)
	{
		Response::sendResponseCode(Response::ERR_NOSUCHCHANNEL, client, {{"channel", channelName}});
		return ;
	}
	if (!channel->isMember(client.getFd()))
	{
		Response::sendResponseCode(Response::ERR_USERNOTINCHANNEL, client, {{"channel", channelName}});
		return ;
	}
	if (!newTopic.empty())
	{
		if (channel->isTopicLocked() && !channel->isOperator(client.getFd()))
		{
			Response::sendResponseCode(Response::ERR_CHANOPRIVSNEEDED, client, {{"channel", channel->getName()}});
			return ;
		}
		channel->setTopic(newTopic);
		broadcastTopic(client, *channel, newTopic);
	}
	else
	{
		if (channel->getTopic().empty())
			Response::sendResponseCode(Response::RPL_NOTOPIC, client, {{"channel", channel->getName()}});
		else
			Response::sendResponseCommand("TOPIC", client, client, {{"channel", channelName}, {"topic", channel->getTopic()}});
	}
}

/* REGISTRATION COMMANDS*/

/// TODO: #4 Implement client timeout when PASS, NICK and USER weren't sent within a timeframe
/**
 * @brief Checks if the user is already authenticated and
 * if the password provided matches the server password.
 * Incorrect password is accepted if the server doesn't require a password in the configuration.
 * Client is only authenticated at the end in case they have filled out user info.
 */
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
		if constexpr ( irc::REQUIRE_PASSWORD )
		{
			client.incrementPassAttempts();
			irc::log_event("AUTH", irc::LOG_FAIL, "incorrect password from " + client.getIpAddress());

			if (client.getPasswordAttempts() >= irc::MAX_PASSWORD_ATTEMPTS)
			{
				Response::sendServerError( client, client.getIpAddress(), "incorrect password");
				client.setActive(false);
				_server.setDisconnectEvent(true);
			}
			return ;
		}
	}

	client.setPassValidated(true);
	irc::log_event("AUTH", irc::LOG_INFO, "valid password from "+ client.getIpAddress());

	CommandHandler::confirmAuth(client);
}

void CommandHandler::handleNick(Client& client, const Command& cmd)
{
	if (cmd.params.empty())
	{
		Response::sendResponseCode(Response::ERR_NONICKNAMEGIVEN, client, {});
		return ;
	}

	std::string newNick = cmd.params[0];

	if ( !CommandHandler::isValidNick(newNick) )
	{
		Response::sendResponseCode(Response::ERR_ERRONEUSNICKNAME, client, {});
		return ;
	}

	for (const auto& [fd, clientObject] : _server.getClients())
	{
		if (clientObject.getNickname() == newNick && static_cast<int>(fd) != client.getFd() )
		{
			if constexpr ( irc::EXTENDED_DEBUG_LOGGING )
				irc::log_event("AUTH", irc::LOG_FAIL, newNick + " already in use");
			Response::sendResponseCode(Response::ERR_NICKNAMEINUSE, client, {{"new nick", newNick}});
			return ;
		}
	}
	if (client.getNickname() != newNick)
	{
		if constexpr ( irc::EXTENDED_DEBUG_LOGGING )
			irc::log_event("AUTH", irc::LOG_INFO, newNick + " set by " + client.getIpAddress());
		client.setNickname(newNick);
	}

	if (!CommandHandler::confirmAuth(client) && client.getPasswordAttempts() >= irc::MAX_PASSWORD_ATTEMPTS)
	{
		Response::sendServerError( client, client.getIpAddress(), "incorrect password");
		client.setActive(false);
		_server.setDisconnectEvent(true);
	}
}

/*
[ \ ] ^ _ ` { | }
*/
/* USER <username> <hostname> <servername> <realname> */
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

	std::string username	= cmd.params[0];
	std::string hostname	= cmd.params[1];
	std::string servername	= cmd.params[2];
	std::string realname	= cmd.params[3];

	if (username.empty() || hostname.empty() || servername.empty() || realname.empty())
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
	client.setHostname(hostname);
	client.setServername(servername);
	client.setRealname(realname);

	if (!CommandHandler::confirmAuth(client) && client.getPasswordAttempts() >= irc::MAX_PASSWORD_ATTEMPTS)
	{
		Response::sendServerError( client, client.getIpAddress(), "incorrect password");
		client.setActive(false);
		_server.setDisconnectEvent(true);
	}
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

	// Broadcast the client disconnection to each member of every channel they were apart of.
	broadcastQuit(client, quitMessage);

	// Send the closing link message to the client
	Response::sendServerError(client, client.getIpAddress(), "QUIT");

	// Client is set as inactive and disconnection event gets announced to the server
	client.setActive(false);
	Server::setDisconnectEvent(true);
}

void CommandHandler::handlePing(Client& client, const Command& cmd)
{
	if (cmd.params.empty())
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "PING"}});
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

/* User and channel mode handling */

void CommandHandler::handleMode(Client& client, const Command& cmd)
{
	if (cmd.params.empty())
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "MODE"}});
		return ;
	}
	const std::string& target = cmd.params[0];
	if (CommandHandler::isChannelName(target))
	{
		handleChannelMode(client, cmd, toLowerCase(target));
	}
	else
	{
		return ;
	}
}
