#include "Response.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "constants.hpp"


/**
 * @brief Relays a command to specified client. Used in broadcasting messages on a channel.
 *
 * Accepted placeholder keys:
 * target, message, channel, reason, new nick, topic, flags
 *
 * @param command The command successfully ran by thhe source client.
 * @param source The client who ran the command.
 * @param target The client who will receive notice on the same channel.
 * @param placeholders Mandatory map of placeholder values.
 */
void	Response::sendResponseCommand( const std::string& command, Client& source, Client& target, const string_map& placeholders )
{
	std::string	templateMessage = getCommandTemplate( command );

	if ( templateMessage.empty() )
		return ;

	string_map fields =
	{
		{ "nick", source.getNickname() },
		{ "user", source.getUsername() },
		{ "host", source.getHostname() },
		{ "target", "" },
		{ "message", "" },
		{ "channel", "" },
		{ "reason", "" },
		{ "new nick", "" },
		{ "topic", "" },
		{ "flags", "" }
	};

	for ( const auto& [placeholder, value] : placeholders )
	{
		if ( placeholder == "target" && fields["reason"].empty() )
			fields["reason"] = value;

		fields[placeholder] = value;
	}

	std::string responseMessage = findAndReplacePlaceholders( templateMessage, fields );

	sendMessage( target, responseMessage );
}

/**
 * @brief Sends a forrmatted response to the client specified by the client fd.
 * Client may be modified if an error occurs when sending a message.
 *
 * Accepted placeholder keys:
 * target, command, channel, users, names, topic, user modes, channel modes,
 * symbol, real name, server info, text, new nick, param, value
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
		fields["target"] = "*";
		fields["command"] = "*";
		fields["channel"] = "*";
		fields["users"] = "*";
		fields["names"] = "*";
		fields["topic"] = "*";
		fields["user modes"] = "***";
		fields["channel modes"] = "***";
		fields["symbol"] = "*";
		fields["real name"] = "*";
		fields["server info"] = "***";
		fields["text"] = "***";
		fields["new nick"] = "*";
		fields["param"] = "*";
		fields["value"] = "***";
	}
	for ( const auto& [placeholder, value] : placeholders )
	{
		fields[placeholder] = value;
	}

	std::string responseMessage = findAndReplacePlaceholders( templateMessage, fields );

	sendMessage( client, responseMessage );
}

/**
 * @brief Sends a buffered partial message to the client.
 *
 * @param client The client which should receive the buffered message.
 */
void	Response::sendPartialResponse( Client& client )
{
	std::string bufferedMessage = client.getSendBuffer();

	if ( bufferedMessage.empty() ) return ;

	client.clearSendBuffer();
	client.setPollout(false);

	sendMessage( client, bufferedMessage );
}


/// Ping-Pong messaging

/**
 * @brief Sends PING message used in checking for inactive clients
 *
 * @param target Who should receive the message.
 * @param token The server getting pinged.
 */
void Response::sendPing( Client& target, const std::string& token )
{
	std::string responseMessage = ":" + _server + " PING " + _server + " :" + token + "\r\n";

	sendMessage( target, responseMessage );
}

/**
 * @brief Sends PONG message used in checking for inactive clients
 *
 * @param target Who should receive the message.
 * @param token The daemon getting ponged (usually the client who is the target).
 */
void Response::sendPong( Client& target, const std::string& token )
{
	std::string responseMessage = ":" + _server + " PONG " + _server + " :" + token + "\r\n";

	sendMessage( target, responseMessage );
}


/// Static member variable initialization and setters

std::string	Response::_date;
std::string	Response::_server;
std::string	Response::_version;

void		Response::setServerDate		( const std::string& date )		{ _date = date; }
void		Response::setServerName		( const std::string& name )		{ _server = name; }
void		Response::setServerVersion	( const std::string& version )	{ _version = version; }


/// Static helper functions

/**
 * @brief Sends a composed message to client.
 * When message couldn't be sent, appends the message to the client send buffer.
 */
void	Response::sendMessage( Client& client, const std::string& message )
{
	ssize_t bytes = send( client.getFd(), message.c_str(), message.length(), MSG_NOSIGNAL );

	if ( bytes < 0 )
	{
		if ( errno == EAGAIN || errno == EWOULDBLOCK )
		{
			if ( client.appendToSendBuffer(message) )
			{
				client.setPollout(true);
				Server::setPolloutEvent(true);
				if ( irc::EXTENDED_DEBUG_LOGGING )
					irc::log_event( "SEND", irc::LOG_DEBUG, "reattempting to send message");
				return ;
			}
		}
		client.setActive(false);
		Server::setDisconnectEvent(true);
		if ( irc::EXTENDED_DEBUG_LOGGING )
			irc::log_event( "SEND", irc::LOG_FAIL, "client has dissconnected");
		return ;
	}
	else if ( bytes < static_cast<ssize_t>(message.length()) )
	{
		if ( client.appendToSendBuffer(message.substr(bytes)) )
		{
			client.setPollout(true);
			Server::setPolloutEvent(true);
			if ( irc::EXTENDED_DEBUG_LOGGING )
				irc::log_event( "SEND", irc::LOG_DEBUG, "partial message sent");
			return ;
		}
		client.setActive(false);
		Server::setDisconnectEvent(true);
		if ( irc::EXTENDED_DEBUG_LOGGING )
			irc::log_event( "SEND", irc::LOG_FAIL, "forcing client disconnection");
		return ;
	}
	if ( irc::EXTENDED_DEBUG_LOGGING )
		irc::log_event( "SEND", irc::LOG_SUCCESS, "message sent successfully");
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
 * @brief Selects the message template for a given command.
 *
 * @param command The command ran in string form.
 * @return Template message if match is found, otherwise empty string.
 */
std::string	Response::getCommandTemplate( const std::string& command )
{
	string_map validCommands =
	{
		{"PRIVMSG", ":<nick>!<user>@<host> PRIVMSG <target> :<message>"},
		{"NOTICE", ":<nick>!<user>@<host> PRIVMSG <target> :<message>"},
		{"JOIN", ":<nick>!<user>@<host> JOIN <channel>"},
		{"PART", ":<nick>!<user>@<host> PART <channel> :<reason>"},
		{"QUIT", ":<nick>!<user>@<host> QUIT :<reason>"},
		{"NICK", ":<nick>!<user>@<host> NICK :<new nick>"},
		{"KICK", ":<nick>!<user>@<host> KICK <channel> <target> :<reason>"},
		{"TOPIC", ":<nick>!<user>@<host> TOPIC <channel> :<topic>"},
		{"MODE", ":<nick>!<user>@<host> MODE <channel> <flags> <target>"},
		{"INVITE", ":<nick>!<user>@<host> INVITE <target> :<channel>"}
	};

	for ( const auto& [key, value] : validCommands )
	{
		if ( key == command )
			return (value);
	}
	return ("");
}

/**
 * @brief Selects the message template for a given response code.
 *
 * @param code The response code to use to fetch a message template.
 * @return If match found then associated message, otherwise empty string.
 */
std::string	Response::getResponseTemplate( int code )
{
	switch ( code )
	{
		/// Connection registration
		case RPL_WELCOME:			return ":<server> <code> <nick> :Welcome to the Internet Relay Network <nick>\r\n";
		case RPL_YOURHOST:			return ":<server> <code> <nick> :Your host is <server>, running version <version>\r\n";
		case RPL_CREATED:			return ":<server> <code> <nick> :This server was created <date>\r\n";
		case RPL_MYINFO:			return ":<server> <code> <nick> <server> <version> <user modes> <channel modes>\r\n";
		case RPL_ISUPPORT:			return ":<server> <code> <nick> <param>=<value> :are supported by this server\r\n";

		case ERR_NONICKNAMEGIVEN:	return ":<server> <code> <nick> :No nickname given\r\n";
		case ERR_ERRONEUSNICKNAME:	return ":<server> <code> <nick> <new nick> :Erroneous nickname\r\n";
		case ERR_NICKNAMEINUSE:		return ":<server> <code> <nick> <new nick> :Nickname is already in use\r\n";
		case ERR_NOTREGISTERED:		return ":<server> <code> <nick> :You have not registered\r\n";
		case ERR_NEEDMOREPARAMS:	return ":<server> <code> <nick> <command> :Not enough parameters\r\n";
		case ERR_ALREADYREGISTRED:	return ":<server> <code> <nick> :You may not reregister\r\n";
		case ERR_PASSWDMISMATCH:	return ":<server> <code> <nick> :Password incorrect\r\n";
		case ERR_TOOMANYCHANNELS:	return ":<server> <code> <nick> <channel> :You have joined too many channels";


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

		case RPL_WHOISUSER:			return ":<server> <code> <nick> <target> <user> <host> * :<real name>\r\n";
		case RPL_WHOISSERVER:		return ":<server> <code> <nick> <target> <server> :<server info>\r\n";
		case RPL_WHOISPERATOR:		return ":<server> <code> <nick> <target> :is an IRC operator\r\n";
		case RPL_ENDOFWHOIS:		return ":<server> <code> <nick> <target> :End of /WHOIS list\r\n";

		/// Message of the day
		case RPL_MOTDSTART:			return "<server> <code> <nick> :- <server> Message of the day -\r\n";
		case RPL_MOTD:				return "<server> <code> <nick> :- <text> -\r\n";
		case RPL_ENDOFMOTD:			return "<server> <code> <nick> :End of /MOTD command\r\n";

		case ERR_NOMOTD:			return "<server> <code> <nick> :MOTD File is missing\r\n";


		/// Disabled features
		case ERR_SUMMONDISABLED:	return "<server> <code> <nick> :SUMMON has been disabled\r\n";
		case ERR_USERSDISABLED:		return "<server> <code> <nick> :USERS has been disabled\r\n";


		default:
			return "";
	}
}
