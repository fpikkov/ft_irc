#include "CommandHandler.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channels.hpp"
#include "Logger.hpp"
#include "Response.hpp"
#include "Command.hpp"
#include "constants.hpp"

/**
 * @brief Broadcast JOIN message to all members of a channel. Also outputs the list of NAMES to the client.
 */
void	CommandHandler::broadcastJoin( Client& client, Channel& channel )
{
	const std::string channelName = channel.getName();
	const auto& allClients = _server.getClients();

	if ( irc::EXTENDED_DEBUG_LOGGING )
		irc::log_event("CHANNEL", irc::LOG_DEBUG, "broadcast: " + channel.getName());

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
		irc::log_event("CHANNEL", irc::LOG_DEBUG, "broadcast: " + channel.getName());

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

void	CommandHandler::broadcastNotice( Client& client, Channel& channel, const std::string& message )
{
	const auto& allClients = _server.getClients();

	for ( auto fd : channel.getMembers() )
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

/**
 * @brief Broadcasts to all members that the user has parted the channel.
 *
 * @param client The client who is departing.
 * @param channel Where the client is departing from.
 * @param message Reason for parting. Empty reason is defaulted to client nickname.
 */
void	CommandHandler::broadcastPart( Client& client, Channel& channel, const std::string& message )
{
	const auto& allClients			= _server.getClients();
	const std::string channelName	= channel.getName();

	for ( const auto memberFd : channel.getMembers() )
	{
		if (memberFd == client.getFd() && !irc::BROADCAST_TO_ORIGIN) continue;

		auto memberIt = allClients.find(memberFd);
		if (memberIt != allClients.end())
		{
			Client& channelMember = const_cast<Client&>(memberIt->second);
			Response::sendResponseCommand("PART", client, channelMember, {{"channel", channelName}, { "reason", message }});
		}
	}
}

/**
 * @brief Broadcasts QUIT message to all channels which the client was apart of.
 * By the nature of the loop, removes the client from the channel clients list.
 * If the client was the only person on the channel, then the channel gets removed.
 *
 * @param client Who is quitting.
 * @param message The optional reason for quitting.
 */
void	CommandHandler::broadcastQuit( Client& client, const std::string& message )
{
	const auto& allClients = _server.getClients();

	for (std::unordered_set<std::string>::iterator it = client.getChannels().begin(); it != client.getChannels().end();)
	{
		Channel* channel = _server.findChannel(*it);
		if (!channel) { ++it; continue; }
		// For every member in this channel
		for (int memberFd : channel->getMembers())
		{
			if (memberFd == client.getFd() && !irc::BROADCAST_TO_ORIGIN) continue;
			// Find member client object by the fd, because Channel class tracks members as a set of file descriptors (int), not as Client objects.
			auto memberIt = allClients.find(memberFd);
			if (memberIt != allClients.end())
			{
				Client& channelMember = const_cast<Client&>(memberIt->second);
				Response::sendResponseCommand("QUIT", client, channelMember, {{ "reason", message }});
			}
		}

		channel->removeMember(client.getFd());
		channel->removeOperator(client.getFd());

		if (channel->isEmpty())
		{
			irc::log_event("CHANNEL", irc::LOG_INFO, "removed: " + channel->getName());
			_server.removeChannel(channel->getName());
			it = client.getChannels().erase(it);
			continue ;
		}
		else { ++it; }
	}
}

/**
 * @brief Broadcasts a channel mode change to all users apart of that channel.
 */
void	CommandHandler::broadcastMode( Client& client, Channel& channel, const std::string& modeStr, const Command& cmd )
{
	const auto& 		allClients	= _server.getClients();
	const std::string	channelName	= channel.getName();
	std::string			target;

	// Append parameters if any
	for (size_t i = 2; i < cmd.params.size(); ++i)
	{
		target += " " + cmd.params[i];
	}

	// Send to all members
	for ( const auto memberFd : channel.getMembers() )
	{
		if (memberFd == client.getFd() && !irc::BROADCAST_TO_ORIGIN) continue;

		auto memberIt = allClients.find(memberFd);
		if (memberIt != allClients.end())
		{
			Client& channelMember = const_cast<Client&>(memberIt->second);
			Response::sendResponseCommand("MODE", client, channelMember, {{"channel", channelName}, {"flags", modeStr}, {"target", target}});
		}
	}
}