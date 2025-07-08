#include "Response.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "constants.hpp"

/**
 * @brief Sends a forrmatted response to the client specified by the client fd.
 * Client may be modified if an error occurs when sending a message.
 *
 * Accepted placeholder keys:
 * target, command, channel, users, names, topic, user_modes, channel_modes, symbol, realname, server_info
 *
 * @param code The response code to send.
 * @param client The client who will receive the response. Nick, user and host placeholders are fetched from this.
 * @param placeholders Optional map of additional placeholder values.
 */
void	Response::sendResponseCode( int code, Client& client, const string_map& placeholders = {} )
{
	std::string	templateMessage = getResponseTemplate( code );

	if ( templateMessage.empty() ) return ;

	string_map fields =
	{
		{ "code", std::to_string(code) },
		{ "nick", client.getNickname() },
		{ "user", client.getUsername() },
		{ "host", client.getHostname() },
		{ "date", _date },
		{ "server", _server },
		{ "version", _version }
	};
	if ( !irc::MEMORY_SAVING )
	{
		fields["target"] = "";
		fields["command"] = "";
		fields["channel"] = "";
		fields["users"] = "";
		fields["names"] = "";
		fields["topic"] = "";
		fields["user_modes"] = "";
		fields["channel_modes"] = "";
		fields["symbol"] = "";
		fields["realname"] = "";
		fields["server_info"] = "";
	}
	for ( const auto& [placeholder, value] : placeholders )
	{
		fields[placeholder] = value;
	}

	std::string responseMessage = findAndReplacePlaceholders( templateMessage, fields );

	// TODO: Mark client for removal if they have disconnected
	sendMessage( client, responseMessage );
}


/// Static member variable setters

void	Response::setServerDate		( const std::string& date )		{ _date = date; }
void	Response::setServerName		( const std::string& name )		{ _server = name; }
void	Response::setServerVersion	( const std::string& version )	{ _version = version; }


/// Static helper functions

bool	Response::sendMessage( Client& client, const std::string& message )
{
	ssize_t bytes = send( client.getFd(), message.c_str(), message.length(), MSG_NOSIGNAL );

	if ( bytes < 0 )
	{
		if ( errno == EAGAIN || errno == EWOULDBLOCK )
		{
			if ( client.appendToSendBuffer(message) )
			{
				// TODO: Change the client's pollfd to POLLOUT and send the buffered message
				return (true);
			}
		}
		// TODO: Mark for removal here
		return (false);
	}
	else if ( bytes < static_cast<ssize_t>(message.length()) )
	{
		if ( client.appendToSendBuffer(message.substr(bytes)) )
		{
			// TODO: Change the client's pollfd to POLLOUT and send the partial buffered message
			return (true);
		}
		// TODO: Mark for removal here
		return (false);
	}
	return (true);
}

std::string	Response::findAndReplacePlaceholders( const std::string& template_string, const string_map& placeholders )
{
	std::string result = template_string;

	for ( const auto& [placeholder, value] : placeholders )
	{
		std::string tag = "<" + placeholder + ">";

		for ( size_t pos = result.find( tag, 0 ); pos != std::string::npos; pos = result.find( tag, pos ) )
		{
			result.replace( pos, tag.length(), value );
			pos += value.length();
		}
	}
	return (result);
}

/**
 * @brief Selects the message template for a given response code.
 *
 * @param code The response code to use to fetch a message template.
 * @return If match found then associated message, otherwise empty string
 */
std::string	Response::getResponseTemplate( int code )
{
	switch ( code )
	{
		/// Connection registration
		case RPL_WELCOME:			return ":<server> <code> <nick> :Welcome to the Internet Relay Network <nick>!<user>@<host>\r\n";
		case RPL_YOURHOST:			return ":<server> <code> <nick> :Your host is <server>, running version <version>\r\n";
		case RPL_CREATED:			return ":<server> <code> <nick> :This server was created <date>\r\n";
		case RPL_MYINFO:			return ":<server> <code> <nick> <server> <version> <user_modes> <channel_modes>\r\n";
		case RPL_ISUPPORT:			return ":<server> <code> <nick> <param>=[<value>] ... :are supported by this server\r\n";

		case ERR_NONICKNAMEGIVEN:	return ":<server> <code> <nick> :No nickname given\r\n";
		case ERR_ERRONEUSNICKNAME:	return ":<server> <code> <nick> <new_nick> :Erroneous nickname\r\n";
		case ERR_NICKNAMEINUSE:		return ":<server> <code> <nick> <new_nick> :Nickname is already in use\r\n";
		case ERR_NOTREGISTERED:		return ":<server> <code> <nick> :You have not registered\r\n";
		case ERR_NEEDMOREPARAMS:	return ":<server> <code> <nick> <command> :Not enough parameters\r\n";
		case ERR_ALREADYREGISTRED:	return ":<server> <code> <nick> :You may not re-register\r\n";
		case ERR_PASSWDMISMATCH:	return ":<server> <code> <nick> :Password incorrect\r\n";


		/// Channel operations
		case RPL_LIST:				return ":<server> <code> <nick> <channel> <users> :<topic>\r\n";
		case RPL_LISTEND:			return ":<server> <code> <nick> :End of /LIST\r\n";
		case RPL_NOTOPIC:			return ":<server> <code> <nick> <channel> :No topic is set\r\n";
		case RPL_NAMREPLY:			return ":<server> <code> <nick> <symbol> <channel> :<names>\r\n";
		case RPL_ENDOFNAMES:		return ":<server> <code> <nick> <channel> :End of /NAMES\r\n";

		case ERR_NOSUCHCHANNEL:		return ":<server> <code> <nick> <channel> :No such channel\r\n";
		case ERR_CHANNELISFULL:		return ":<server> <code> <nick> <channel> :Channel is full\r\n";
		case ERR_INVITEONLYCHAN:	return ":<server> <code> <nick> <channel> :Invite only channel\r\n";
		case ERR_BANNEDFROMCHAN:	return ":<server> <code> <nick> <channel> :Banned from channel\r\n";
		case ERR_BADCHANNELKEY:		return ":<server> <code> <nick> <channel> :Bad channel key\r\n";

		case ERR_USERNOTINCHANNEL:	return ":<server> <code> <nick> <target> <channel> :User not in channel\r\n";
		case ERR_NOTONCHANNEL:		return ":<server> <code> <nick> <channel> :You're not on that channel\r\n";
		case ERR_USERONCHANNEL:		return ":<server> <code> <nick> <target> <channel> :User already on channel\r\n";


		/// Message handling
		case ERR_NOSUCHNICK:		return ":<server> <code> <nick> :No such nickname\r\n";
		case ERR_CANNOTSENDTOCHAN:	return ":<server> <code> <nick> :Cannot send to channel\r\n";
		case ERR_NOTEXTTOSEND:		return ":<server> <code> <nick> :No text to send\r\n";
		case ERR_UNKNOWNCOMMAND:	return ":<server> <code> <nick> <command> :Unknown command\r\n";

		case RPL_WHOISUSER:			return ":<server> <code> <nick> <target> <user> <host> * :<realname>\r\n";
		case RPL_WHOISSERVER:		return ":<server> <code> <nick> <target> <server> :<server_info>\r\n";
		case RPL_WHOISPERATOR:		return ":<server> <code> <nick> <target> :is an IRC operator\r\n";
		case RPL_ENDOFWHOIS:		return ":<server> <code> <nick> <target> :End of /WHOIS list\r\n";


		default:
			return "";
	}
}
