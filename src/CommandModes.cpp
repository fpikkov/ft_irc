#include "CommandHandler.hpp"
#include "Channels.hpp"
#include "Client.hpp"
#include "Response.hpp"
#include "Server.hpp"
#include "Command.hpp"
#include "constants.hpp"


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
		sendChannelModeReply(client, channel, channelName, cmd); //this function needs to broadcast. Broadcasting requires cmd and paramindex as additional parameters.
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

void CommandHandler::sendChannelModeReply(Client& client, Channel* channel, const std::string& channelName, [[maybe_unused]] const Command& cmd)
{
	std::string modes = "+";
	std::string params;

	if (channel->isInviteOnly()) modes += "i";
	if (channel->isTopicLocked()) modes += "t";
	if (!channel->getKey().empty()) // if key not empty
	{
		modes += "k";
		params += " " + channel->getKey(); // add key to params
	}
	if (channel->getUserLimit())
	{
		modes += "l";
		params += " " + std::to_string(channel->getUserLimit());
	}
	// TODO: #10 Check where we should be broadcasting the mode to the channel
	// broadcastMode(client, *channel, params, cmd);
	Response::sendResponseCommand("MODE", client, client, {{"channel", channelName}, {"flags", modes}, {"target", params}});
}

// params[0]=channel, params[1]=modes, params[2...]=params
// MODE #mychan +itk secretkey -o Alice +l 50

void CommandHandler::parseAndApplyChannelModes(Client& client, Command& cmd, Channel* channel, const std::string& channelName)
{
	std::string modeStr = cmd.params[1];
	size_t paramIndex = 2;

	bool adding = true;

	//TODO: #12 There is something wrong with modes +i, +t, +k. The attributes of channel are not affected by these.
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
	broadcastMode(client, *channel, modeStr, cmd);
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
