#include "Response.hpp"
#include "Server.hpp"
#include "Client.hpp"

void	Response::sendResponseCode( int code, Client& client, Server& server )
{

}
void	Response::sendResponseCode( int code, Client& client, Server& server, Channel& channel )
{

}

void	Response::sendResponseCommand( int code, Client& client, Server& server, const std::string& message )
{

}
void	Response::sendResponseCommand( int code, Client& client, Server& server, Channel& channel, const std::string& message )
{

}


/**
 * @brief Selects the default response message for error replies
 * @return If match found then associated message, otherwise empty string
 */
std::string	Response::getResponseMessage( int code )
{
	switch ( code )
	{
		/// Connection registration
		case RPL_WELCOME:			return ":Welcome to the Internet Relay Network <nick>!<user>@<host>";
		case RPL_YOURHOST:			return ":Your host is <server>, running version <version>";
		case RPL_CREATED:			return ":This server was created <date>";
		case RPL_MYINFO:			return "<server> <version> <channel_modes>";
		case RPL_ISUPPORT:			return "<param>=[<value>] ... :are supported by this server";

		case ERR_NONICKNAMEGIVEN:	return ":No nickname given";
		case ERR_ERRONEUSNICKNAME:	return ":Erroneous nickname";
		case ERR_NICKNAMEINUSE:		return ":Nickname is already in use";
		case ERR_NOTREGISTERED:		return ":You have not registered";
		case ERR_NEEDMOREPARAMS:	return ":Not enough parameters";
		case ERR_ALREADYREGISTRED:	return ":You may not re-register";
		case ERR_PASSWDMISMATCH:	return ":Password incorrect";


		/// Channel operations
		case RPL_LIST:				return "";
		case RPL_LISTEND:			return "";
		case RPL_NOTOPIC:			return "";
		case RPL_NAMREPLY:			return "";
		case RPL_ENDOFNAMES:		return "";

		case ERR_NOSUCHCHANNEL:		return ":No such channel";
		case ERR_CHANNELISFULL:		return ":Channel is full";
		case ERR_INVITEONLYCHAN:	return ":Invite only channel";
		case ERR_BANNEDFROMCHAN:	return ":Banned from channel";
		case ERR_BADCHANNELKEY:		return ":Bad channel key";

		case ERR_USERNOTINCHANNEL:	return ":User not in channel";
		case ERR_NOTONCHANNEL:		return ":You're not on that channel";
		case ERR_USERONCHANNEL:		return ":User already on channel";


		/// Message handling
		case ERR_NOSUCHNICK:		return ":No such nickname";
		case ERR_CANNOTSENDTOCHAN:	return ":Cannot send to channel";
		case ERR_NOTEXTTOSEND:		return ":No text to send";
		case ERR_UNKNOWNCOMMAND:	return ":Unknown command";

		case RPL_WHOISUSER:			return "";
		case RPL_WHOISSERVER:		return "";
		case RPL_WHOISPERATOR:		return "";
		case RPL_ENDOFWHOIS:		return "";


		default:
			return ":Unknown response code";
	}
}
