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

	size_t	paramIndex = 2;

	// Check the first parameter AKA mode string
	if ( !constructModeNodes(client, cmd, cmd.params[1], paramIndex, modes) )
		return false;

	// Look for trailing modes
	while ( paramIndex < cmd.params.size() )
	{
		if ( !cmd.params[paramIndex].empty() )
		{
			if ( !constructModeNodes(client, cmd, cmd.params[paramIndex++], paramIndex, modes) )
				return false;
		}
		else
		{
			++paramIndex;
		}
	}
	return true;
}

bool	CommandHandler::constructModeNodes( Client& client, const Command& cmd, const std::string& tokens, size_t& paramIndex, std::vector<Mode>& modes )
{
	bool	adding = true;
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
			bool paramRequired = CommandHandler::requiresParam(mode, adding);
			if ( paramRequired )
			{
				if ( idx + 1 < tokens.length() )
				{
					if ( mode == 'l' && isdigit(tokens[idx + 1]) )
					{
						param = CommandHandler::inlineParam(tokens, idx, [](char c) { return std::isdigit(c); });
					}
					else if ( (mode == 'k' || mode == 'o') && !CommandHandler::isSign(tokens[idx + 1]) )
					{
						param = CommandHandler::inlineParam(tokens, idx, [](char c) { return !CommandHandler::isSign(c); });
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
	return true;
}
//----------------------------------------------------------------------------------------------------------------------------------

//MODE #channel +l20k-k -key -o Alice

// TODO: #15 The handlers can be substituted with the short code they contain.

/**
 * @brief Applies a pre-validated plan of mode changes and broadcasts the result.
 *
 * This function is the second pass of a two-pass system. It should only ever
 * receive a vector of modes that have been fully validated by the parser.
 * Its sole responsibility is to apply the changes and create an accurate
 * report of what was changed, which is then broadcast to the channel.
 *
 * @param client The client initiating the mode change.
 * @param channel The channel on which modes are being changed.
 * @param modes A vector of pre-validated Mode structures to apply.
 */
void CommandHandler::applyChannelModes(Client& client, Channel& channel, std::vector<Mode>& modes)
{
	// 1. Precheck 
	if (modes.empty())
		return;
	// 2. Setting up strings to build the broadcast message
	std::string appliedModeStr;
	std::string appliedParams;

	bool signHasBeenSet = false;
	bool currentSign = true; // This bool tracks the last sign (+/-) added to the mode string
	bool failureState = false;

	// 3. Executing and broadcasting.
	for (Mode& currentMode : modes)
	{
		// Applying the change
		switch (currentMode.mode)
		{
			case 'i': channel.setInviteOnly(currentMode.adding); break;
			case 't': channel.setTopicLocked(currentMode.adding); break;
			case 'k': channel.setKey(currentMode.adding ? currentMode.param : ""); break;
			case 'l':
				if (currentMode.adding)
				{
					size_t limit = std::stoul(currentMode.param);
					if (limit > irc::MAX_CHANNELS)
					{
						limit = irc::MAX_CHANNELS;
						currentMode.param = std::to_string(irc::MAX_CHANNELS);
					}
					
					channel.setUserLimit(limit);
				}
				else
				{
					channel.setUserLimit(0);
				}
				break;
			case 'o':
				{
					Client* target = _server.findUser(currentMode.param);
					if (target)
					{
						if (currentMode.adding) channel.addOperator(target->getFd());
						else channel.removeOperator(target->getFd());
					}
					else
					{
						failureState = true;
					}
					break;
				}
		}
		if (failureState)
		{
			failureState = false;
			continue;
		}
		// Adding changes to report strings
		if (!signHasBeenSet || currentMode.adding != currentSign)
		{
			currentSign = currentMode.adding;
			appliedModeStr += (currentSign ? '+' : '-');
			signHasBeenSet = true;
		}
		appliedModeStr += currentMode.mode;

		if (!currentMode.param.empty())
		{
			appliedParams += " ";
			appliedParams.append(currentMode.param);
		}
	}

	broadcastMode(client, channel, appliedModeStr + appliedParams);
}
