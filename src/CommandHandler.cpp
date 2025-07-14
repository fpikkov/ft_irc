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
		if ( irc::EXTENDED_DEBUG_LOGGING )
			irc::log_event("COMMAND", irc::LOG_INFO, cmd.command + " from " + client.getNickname() + "@" + client.getIpAddress());
		it->second(client, cmd);
	}
	else
	{
		if ( irc::EXTENDED_DEBUG_LOGGING )
			irc::log_event("COMMAND", irc::LOG_FAIL, "unknown " + cmd.command + " from " + client.getNickname() + "@" + client.getIpAddress());
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
	if (client.getChannels().size() > irc::MAX_CHANNELS)
	{
		Response::sendResponseCode(Response::ERR_TOOMANYCHANNELS, client, {{"channel", cmd.params[0]}});
		return ;
	}

	std::string	target	= toLowerCase(cmd.params[0]); //Channel names are stored in lowercase..
	std::string	key		= (cmd.params.size() > 1) ? cmd.params[1] : "";

	Channel* channel	= _server.findChannel(target);

	if (!channel)
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
		if (channel->addMember(client.getFd()) == true)
		{
			irc::log_event("CHANNEL", irc::LOG_INFO, client.getNickname() + "@" + client.getIpAddress() + " joined " + target);
		}
		else
		{
			if (channel->isEmpty())
			{
				irc::log_event("CHANNEL", irc::LOG_INFO, "removed: " + target);
				_server.removeChannel(channel->getName());
				return ;
			}
		}
		channel->addOperator(client.getFd());

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

	// Remove user from the channel
	irc::log_event("CHANNEL", irc::LOG_INFO, client.getNickname() + "@" + client.getIpAddress() + " left " + channelName);
	channel->removeMember(client.getFd());

	// Remove the channel if no members exist
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
	std::string	comment = (cmd.params.size() > 2) ? cmd.params[3] : "";

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
	//broadcast the message a member has been kicked
	//if comment.empty() = false, join the comment with the default message.
}

void	CommandHandler::handleInvite(Client& client, const Command& cmd)
{
	if (!client.isAuthenticated())
	{
		Response::sendResponseCode(Response::ERR_NOTREGISTERED, client, {});
		return ;
	}
	if (cmd.params.size() != 2)
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "INVITE"}});
		return ;
	}

	std::string	channelName = toLowerCase(cmd.params[1]);

	Client* target = _server.findUser(cmd.params[0]);
	Channel* channel = _server.findChannel(channelName);

	if (!target)
	{
		Response::sendResponseCode(Response::ERR_NOSUCHNICK, client, {{"target", target->getNickname()}});
		return ;
	}
	if (!channel)
	{
		Response::sendResponseCode(Response::ERR_NOSUCHCHANNEL, client, {{"channel", channel->getName()}});
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
	//send INVITE message to the target user.
	//OPTIONAL: Send a confirmation to the user who sent invite
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
		//broadcast to all users that topic has been changed.
	}
	else
	{
		//message the current topic to the requester.
		//if no topic is set, send "no topic" message.
	}
}

/* REGISTRATION COMMANDS*/

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
		irc::log_event("AUTH", irc::LOG_FAIL, "incorrect password from " + client.getIpAddress());
		Response::sendResponseCode(Response::ERR_PASSWDMISMATCH, client, {});
		if ( irc::REQUIRE_PASSWORD )
			return ;
	}

	irc::log_event("AUTH", irc::LOG_SUCCESS, "valid password from "+ client.getIpAddress());
	if ( !client.getUsername().empty() && !client.getHostname().empty() && !client.getRealname().empty() )
		client.setAuthenticated(true);
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
		if (clientObject.getNickname() == newNick)
		{
			if ( irc::EXTENDED_DEBUG_LOGGING )
				irc::log_event("AUTH", irc::LOG_FAIL, newNick + " already in use");
			Response::sendResponseCode(Response::ERR_NICKNAMEINUSE, client, {});
			return ;
		}
	}
	if ( irc::EXTENDED_DEBUG_LOGGING )
		irc::log_event("AUTH", irc::LOG_SUCCESS, newNick + " set by " + client.getIpAddress());
	client.setNickname(newNick);
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
	client.setAuthenticated(true);

	irc::log_event("AUTH", irc::LOG_SUCCESS, client.getNickname() + "@" + client.getIpAddress() + " authenticated");
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


// ======================================================================================================================================================================================================== //

/* User and channel mode handling */

void CommandHandler::handleMode(Client& client, const Command& cmd)
{
	if (cmd.params.empty())
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "MODE"}});
	}
	const std::string& target = cmd.params[0];
	if (isChannelName(target))
	{
		handleChannelMode(client, cmd, toLowerCase(target));
	}
	else
	{
		//handlig UserMode
		Response::sendResponseCode(Response::ERR_NOSUCHCHANNEL, client, {{"command", target}});
	}
}

void CommandHandler::handleChannelMode(Client& client, const Command& cmd, const std::string& channelName)
{
	Channel* channel = _server.findChannel(channelName); // A pointer here because a channel might not exist (will return a nullptr in this case). A reference would not work here, as reference must always refer to a valid object
	if (!channel)
	{
		Response::sendResponseCode(Response::ERR_NOSUCHCHANNEL, client, {{"channel", channelName}});
		return ;
	}

	//Gettign modes of the channel (querying) params[0] = #channel, params[1] = mode string (+i, -k, +o etc.)
	if (cmd.params.size() == 1)
	{
		sendChannelModeReply(client, channel, channelName);
		return;
	}

	// Checking operator priviliges
	if (!channel->isOperator(client.getFd()))
	{
		Response::sendResponseCode(Response::ERR_CHANOPRIVSNEEDED, client, {{"channel", channelName}});
		return ;
	}

	parseAndApplyChannelModes(client, const_cast<Command&>(cmd), channel, channelName);

}

void CommandHandler::sendChannelModeReply(Client& client, Channel* channel, const std::string& channelName)
{
	std::string modes = "+";
	std::string params;

	if (channel->isInviteOnly()) modes += "i";
	if (channel->isTopicLocked()) modes += "t";
	if (channel->getKey()) // if not a nullptr
	{
		modes += "k";
		params += " " + *channel->getKey(); // dereferencing
	}
	if (channel->getUserLimit())
	{
		modes += "l";
		params += " " + std::to_string(channel->getUserLimit());
	}
	// std::string reply = ":" + _server.getServerHostname() + "MODE" + channelName + " " + modes + params + "\r\n";
	//Sending message here (client, reply);
	// TODO: check if the message should be broadcasted to the whole channel
	Response::sendResponseCommand("MODE", client, client, {{"channel", channelName}, {"flags", modes}, {"target", params}});
}

// params[0]=channel, params[1]=modes, params[2...]=params
// MODE #mychan +itk secretkey -o Alice +l 50

void CommandHandler::parseAndApplyChannelModes(Client& client, Command& cmd, Channel* channel, const std::string& channelName)
{
	std::string modeStr = cmd.params[1];
	size_t paramIndex = 2;

	bool adding = true;

	for (size_t i; i < modeStr.size(); ++i)
	{
		char mode = modeStr[i];
		if (mode == '+') { adding = true; continue; }
		if (mode == '-') { adding = false; continue; }
		switch (mode)
		{
			case 'i': handleModeInviteOnly(channel, adding); break;
			case 't': handleModeTopicLocked(channel, adding); break;
			case 'k': handleModeKey(client, cmd, channel, adding, paramIndex); break;
			case 'l': handleModeLimit(client, cmd, channel, adding, paramIndex); break;
			case 'o': handleModeOperator(client, cmd, channel, adding, paramIndex, channelName); break;
			default: break;
		}
	}
	broadcastMode(client, *channel, modeStr, cmd, paramIndex);
}


void CommandHandler::handleModeInviteOnly(Channel* channel, bool adding)
{
	channel->setInviteOnly(adding);
}

void CommandHandler::handleModeTopicLocked(Channel* channel, bool adding)
{
	channel->setTopicLocked(adding);
}

void CommandHandler::handleModeKey(Client& client, Command& cmd, Channel* channel, bool adding, size_t& paramIndex)
{
	if (adding)
	{
		if (paramIndex >= cmd.params.size())
		{
			Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "MODE"}});
			return ;
		}
		channel->setKey(cmd.params[paramIndex]);
		++paramIndex;
	}
	else
	{
		channel->setKey("");
	}
}

void CommandHandler::handleModeLimit(Client& client, Command& cmd, Channel* channel, bool adding, size_t& paramIndex)
{
	if (adding)
	{
		if (paramIndex >= cmd.params.size())
		{
			Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "MODE"}});
			return ;
		}
		int limit = std::atoi(cmd.params[paramIndex].c_str());
		channel->setUserLimit(limit);
		++paramIndex;
	}
	else
	{
		channel->setUserLimit(0);
	}
}

void CommandHandler::handleModeOperator(Client& client, Command& cmd, Channel* channel, bool adding, size_t& paramIndex, const std::string& channelName)
{
	if (paramIndex >= cmd.params.size())
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "MODE"}});
		return ;
	}
	Client* targetUser = _server.findUser(cmd.params[paramIndex]);
	if (!targetUser || !channel->isMember(targetUser->getFd()))
	{
		Response::sendResponseCode(Response::ERR_USERNOTINCHANNEL, client, {{"target", cmd.params[paramIndex]}, {"channel", channelName}});
		return ;
	}
	if (adding)
	{
		channel->addOperator(targetUser->getFd());
	}
	else
	{
		channel->removeOperator(targetUser->getFd());
	}
	++paramIndex;
}


