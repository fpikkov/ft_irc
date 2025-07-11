/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rkhakimu <rkhakimu@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/08 13:17:30 by ahentton          #+#    #+#             */
/*   Updated: 2025/07/11 17:07:54 by rkhakimu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CommandHandler.hpp"
#include "Channels.hpp"
#include "Client.hpp"
#include "Response.hpp"
#include "Server.hpp"
#include "Command.hpp"
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

static	std::string	stringToLower(const std::string channelName)
{
	std::string result = channelName;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
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

	if (target[0] == '#' || target[0] == '&')
	{
		target = stringToLower(target); //Channels are stored in lowercase.
		Channel *channel = _server.findChannel(target);
		if (!channel)
		{
			Response::sendResponseCode(Response::ERR_NOSUCHCHANNEL, client, {{"channel", target}});
			return ;
		}
		//Send to all clients who belong in the channel.
	}
	else
	{
		Client	*recipient = _server.findUser(target);
		if (!recipient)
		{
			Response::sendResponseCode(Response::ERR_NOSUCHNICK, client, {{"nick", target}});
			return ;
		}
		//Send to the recipient client
	}
}

/* NOTE: NOTICE is very much like PRIVMSG, but it does not care about errors.
   No replies are expected from either the server or the recipient.*/
   
void	CommandHandler::handleNotice(Client& client, const Command& cmd)
{
	if (!client.isAuthenticated())
	{
		Response::sendResponseCode(Response::ERR_NOTREGISTERED, client, {});
		return ;
	}
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
	if (client.getChannels().count() > USER_MAX_CHANNELS) //TODO: Add constant for max channels.
	{
		Response::sendResponseCode(Response::ERR_TOOMANYCHANNELS, client, {{"channel", cmd.params[0]}}); //TODO: Add response for toomanychannels.
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
		//Send confirmation to the client they joined the channel.
		//Client does not need confirmation that they were made operator in this instance.
		return ;
	}
	
	if (channel->isFull())
	{
		Response::sendResponseCode(Response::ERR_CHANNELISFULL, client, {{"channel", channel->getName()}});
		return ;
	}
	
	if (channel->isInviteOnly())
	{
		if (channel->isInvited(client.getFd()))
		{
			if (channel->addMember(client.getFd() == true)
				//send confirmation user has joined the channel.
		}
		else
			Response::sendResponseCode(Response::ERR_INVITEONLYCHAN, client, {{"channel", channel->getName()}});
		return ;
	}
	
	if (channel->getKey() && key.empty() || channel->getKey() != key)
	{
		Response::sendResponseCode(Response::ERR_BADCHANNELKEY, client, {{"channel", channel->getName()}});
		return ;
	}
	else
	{
		if (channel->addMember(client.getFd()) == true)
			//Send confirmation to the client they joined a channel.
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
	std::string	comment = (cmd.params.size() > 1) ? cmd.params[1] : "";
	
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
	
	//Broadcast a parting message to all users in the channel, and the user who is leaving.
	//if (!comment.isEmpty()): Conconate the default parting message and comment
	channel->removeMember(client.getFd());
	if (channel->isEmpty())
	{
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
		Response::sendResponseCode(Response::ERR_NOSUCHNICK, client, {{"target", target->getNickname()}});
		return ;
	}
	if (!channel->isOperator(client.getFd()))
	{
		//TODO: add ERR_CHANOPRIVSNEEDED response.
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

	std::string	channelName = stringToLower(cmd.params[1]);

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

	std::string	channelName = stringToLower(cmd.params[0]);
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

void CommandHandler::handleQuit(Client& client, const Command& cmd)
{
	std::string quitMessage = "AWOLNATION - RUN";
	if (!cmd.params.empty())
	{
		quitMessage = cmd.params[0];
		if (!quitMessage.empty() && quitMessage[0] == ':')
			quitMessage = quitMessage.substr(1);
	}
	
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
			if (memberFd == client.getFd()) continue;
			// Find member client object by the fd, because Channel class tracks members as a set of file descriptors (int), not as Client objects.
			// In order to be able to send a message we need Client object, not just fd.
			const auto& allClients = _server.getClients();
			auto memberIt = allClients.find(memberFd);
			if (memberIt != allClients.end())
			{
				Client& channelMember = const_cast<Client&>(memberIt->second);
				//sending message here (Mr. @Fpikkov)
			}
		}
		channel->removeMember(client.getFd());
	}
	client.setActive(false);
}

// TODO broadcasting automation.

void CommandHandler::handlePing(Client& client, const Command& cmd)
{
	if (cmd.params.empty())
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "USER"}}); // The outer { ... } is for the map initializer. The inner { ... } is for each key-value pair.
		return ;
	}
	std::string token = cmd.params[0];
	//sending PONG reply?
	std::string replyPong = "PONG" + token +"\r\n";
}

void CommandHandler::handlePong(Client& client, const Command& cmd)
{
	if (cmd.params.empty())
	{
		Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "USER"}}); // The outer { ... } is for the map initializer. The inner { ... } is for each key-value pair.
		return ;
	}
}


// ======================================================================================================================================================================================================== //

// Handle MODE part

bool CommandHandler::isChannelName(const std::string& name) const
{
	return !name.empty() && (name[0] == '#' || name[0] == '&');
}

std::string CommandHandler::toLowerCase(const std::string& s) const
{
	std::string result = s;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

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
		Response::sendResponseCode(Response::ERR_CHANOPRIVSNEEDED (482), client, {{"channel", channelName}});
		return ;
	}

	parseAndApplyChannelModes(client, cmd, channel, channelName);
	
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
	std::string reply = ":" + _server.getServerHostname() + "MODE" + channelName + " " + modes + params + "\r\n";
	//Sending message here (client, reply);
}

// params[0]=channel, params[1]=modes, params[2...]=params
// MODE #mychan +itk secretkey -o Alice +l 50

void CommandHandler::parseAndApplyChannelModes(Client& client, Command& cmd, Channel* channel, const std::string& channelName)
{
	std::string modeStr = cmd.params[1];
	size_t papamIndex = 2;

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
			case 'k': handleModeKey(client, cmd, channel, adding, papamIndex); break;
			case 'l': handleModeLimit(client, cmd, channel, adding, papamIndex); break;
			case 'o': handleModeOperator(client, cmd, channel, adding, papamIndex, channelName); break;
			default: break;
		}
	}
	broadcastChannelModeChange(client, channel, channelName, modeStr, cmd, papamIndex);
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

void CommandHandler::broadcastChannelModeChange(Client& client, Channel* channel, const std::string& channelName, const std::string& modeStr, const Command& cmd, size_t paramIndex)
{
    std::string broadcast = ":" + client.getNickname() + "!" + client.getUsername() + "@" + client.getHostname() + " MODE " + channelName + " " + modeStr;
    // Append parameters if any
    for (size_t i = 2; i < paramIndex && i < cmd.params.size(); ++i)
	{
        broadcast += " " + cmd.params[i];
    }
    broadcast += "\r\n";
    // Send to all members
	//Client* memberFd = _server.findUser(cmd.params[paramIndex]);
    for (auto memberFd : channel->getMembers())
	{
        Client* member = _server.findUser(std::to_string(memberFd));
        //if (member)
           //sending message
    }
}