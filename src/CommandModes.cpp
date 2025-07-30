#include "CommandHandler.hpp"
#include "Channels.hpp"
#include "Client.hpp"
#include "Response.hpp"
#include "Server.hpp"
#include "Command.hpp"
#include "constants.hpp"
#include "Mode.hpp"


void CommandHandler::handleChannelMode(Client& client, const Command& cmd, const std::string& channelName)
{
	Channel* channel = _server.findChannel(channelName); // A pointer here because a channel might not exist (will return a nullptr in this case). A reference would not work here, as reference must always refer to a valid object
	if (!channel)
	{
		Response::sendResponseCode(Response::ERR_NOSUCHCHANNEL, client, {{"channel", channelName}});
		return ;
	}

	//Getting modes of the channel (querying) params[0] = #channel, params[1] = mode string (+i, -k, +o etc.)
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

	std::vector<Mode> modes;

	// Parsing modes intoo their own structure
	if (!parseChannelModes(client, cmd, modes))
		return ;
	// Apply modes from parsed structure
	applyChannelModes(client, *channel, modes);
}

void CommandHandler::sendChannelModeReply(Client& client, Channel* channel, const std::string& channelName)
{
	std::string modes = "+";
	std::string params;

	if (channel->isInviteOnly()) modes += "i";
	if (channel->isTopicLocked()) modes += "t";
	if (!channel->getKey().empty()) // if key not empty
	{
		modes += "k";
		params += channel->getKey(); // add key to params
	}
	if (channel->getUserLimit())
	{
		modes += "l";
		if (!params.empty())
			params += " ";
		params += std::to_string(channel->getUserLimit());
	}
	Response::sendResponseCode(Response::RPL_CHANNELMODEIS, client, {{"channel", channelName}, {"mode", modes}, {"mode params", params}});
}


// TODO: #16 Refactor parseChannelModes into multiple functions
/**
 * @brief Parses the MODE command into a separate struct which keeps track of mode type, parameters and if the mode is being added.
 * Modes which require parameters: +kol, -o
 *
 * @param cmd The command structure.
 * @param[out] modes Vector of the mode struct.
 * @return True on successful data parsing, otherwise false.
 */
bool	CommandHandler::parseChannelModes(Client& client, const Command& cmd, std::vector<Mode>& modes)
{
	if ( cmd.params.size() < 2 )
		return false;

	modes.clear();

	std::string	tokens = cmd.params[1];
	size_t		paramIndex = 2;
	bool		adding = true;

	for ( size_t idx = 0; idx < tokens.length(); ++idx )
	{
		char		mode = tokens[idx];
		std::string	param;

		switch (mode)
		{
			case '+': adding = true; continue;
			case '-': adding = false; continue;
			default: break;
		}

		if ( CommandHandler::isMode(mode) )
		{
			// Find parameters for modes which require them
			if ( mode == 'o' || ( mode == 'k' && adding ) || ( mode == 'l' && adding ) )
			{
				if ( idx + 1 < tokens.length() )
				{
					if ( mode == 'l' && isdigit(tokens[idx + 1]) )
					{
						while ( idx + 1 < tokens.length() && isdigit(tokens[idx + 1]) )
						{
							param += tokens[idx + 1];
							++idx;
						}
					}
				}
				else if ( (mode == 'k' || mode == 'o') && !CommandHandler::isSign(tokens[idx + 1]) )
				{
					while ( idx + 1 < tokens.length() && !CommandHandler::isSign(tokens[idx + 1]) )
					{
						param += tokens[idx + 1];
						++idx;
					}
				}

				// Check for trailing parameters if none were found inline
				if ( param.empty() )
				{
					if ( paramIndex >= cmd.params.size() || cmd.params[paramIndex].empty() )
					{
						Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "MODE"}});
						return false;
					}
					param = cmd.params[paramIndex++];
				}
			}
			modes.emplace_back(mode, param, adding);
		}
		else
		{
			Response::sendResponseCode(Response::ERR_UNKNOWNMODE, client, {{"mode", std::to_string(mode)}});
			return false;
		}
	}

	// Check for trailing modes which should NOT contain any parameters (how tf would you parse that?)
	while ( paramIndex < cmd.params.size() )
	{
		adding = true;
		tokens = cmd.params[paramIndex];
		if ( !cmd.params[paramIndex].empty() )
		{
			for ( size_t idx = 0; idx < tokens.length(); ++idx )
			{
				char	mode = tokens[idx];
				switch (mode)
				{
					case '+': adding = true; continue;
					case '-': adding = false; continue;
					default: break;
				}

				if ( ( mode == 'k' && !adding ) || ( mode == 'l' && !adding ) || mode == 'i' || mode == 't' )
				{
					modes.emplace_back(mode, "", adding);
				}
				else
				{
					Response::sendResponseCode(Response::ERR_NEEDMOREPARAMS, client, {{"command", "MODE"}});
					return false;
				}
			}
		}
		++paramIndex;
	}
	return true;
}

// TODO: #15 The handlers can be substituted with the short code they contain.
void	CommandHandler::applyChannelModes(Client& client, Channel& channel, const std::vector<Mode>& modes)
{
	// TODO: #14 When a MODE change partially fails, construct a new modeStr for the clients with valid commands.
	std::string modeStr;
	std::string paramStr;


	for ( const auto& currentMode : modes )
	{
		switch (currentMode.mode)
		{
			case 'i': handleModeInviteOnly(channel, currentMode.adding); break;
			case 't': handleModeTopicLocked(channel, currentMode.adding); break;
			case 'k': handleModeKey(channel, currentMode.adding, currentMode); break;
			case 'l': handleModeLimit(channel, currentMode.adding, currentMode); break;
			case 'o': handleModeOperator(client, channel, currentMode.adding, currentMode); break;
			default: break;
		}
	}
	broadcastMode(client, channel, modeStr);
}


void CommandHandler::handleModeInviteOnly(Channel& channel, bool adding)
{
	channel.setInviteOnly(adding);
}

void CommandHandler::handleModeTopicLocked(Channel& channel, bool adding)
{
	channel.setTopicLocked(adding);
}

void CommandHandler::handleModeKey(Channel& channel, bool adding, const Mode& mode)
{
	// NOTE: Useless since mode.param will be empty when not adding
	if (adding)
	{
		channel.setKey(mode.param);
	}
	else
	{
		channel.setKey("");
	}
}

void CommandHandler::handleModeLimit(Channel& channel, bool adding, const Mode& mode)
{
	if (adding)
	{
		try
		{
			size_t limit = std::stoul(mode.param);
			if (limit > irc::MAX_CHANNELS)
				limit = irc::MAX_CHANNELS;
			channel.setUserLimit(limit);
		}
		catch (...)
		{
			// Do nothing as we got an invalid argument
		}
	}
	else
	{
		channel.setUserLimit(0); // Set limit to 0 to unset
	}
}

void CommandHandler::handleModeOperator(Client& client, Channel& channel, bool adding, const Mode& mode)
{
	Client* targetUser = _server.findUser(mode.param);
	if (!targetUser || !channel.isMember(targetUser->getFd()))
	{
		Response::sendResponseCode(Response::ERR_USERNOTINCHANNEL, client, {{"target", mode.param}, {"channel", channel.getName()}});
		return ;
	}
	if (adding)
	{
		channel.addOperator(targetUser->getFd());
	}
	else
	{
		channel.removeOperator(targetUser->getFd());
	}
}
