#include "Response.hpp"
#include "Server.hpp"
#include "Client.hpp"

#include <map>

/**
 * @brief Will go through a list of template messages and replace placeholders within the template
 * with fields containing the correct data for response construction.
 * Sends over a response code to the Client's fd.
 */
void	Response::sendResponseCode( int code, Client& client, Server& server )
{
	std::string	templateMessage = getResponseMessage( code );

	if ( templateMessage.empty() ) return ;

	// Fill in more required fields as needed
	std::map<std::string, std::string> fields =
	{
		{ "code", std::to_string(code) },
		{ "nick", client.getNickname() },
		{ "user", client.getUsername() },
		{ "host", client.getHostname() },
		{ "date", server.getServerStartTime() },
		{ "server", server.getServerHostname() },
		{ "version", server.getServerVersion() }
	};

	// TODO: Replace the fields in the template string


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
		case RPL_WELCOME:			return ":<server> <code> <nick> :Welcome to the Internet Relay Network <nick>!<user>@<host>";
		case RPL_YOURHOST:			return ":<server> <code> <nick> :Your host is <server>, running version <version>";
		case RPL_CREATED:			return ":<server> <code> <nick> :This server was created <date>";
		case RPL_MYINFO:			return ":<server> <code> <nick> <server> <version> <user_modes> <channel_modes>";
		case RPL_ISUPPORT:			return ":<server> <code> <nick> <param>=[<value>] ... :are supported by this server";

		case ERR_NONICKNAMEGIVEN:	return ":<server> <code> <nick> :No nickname given";
		case ERR_ERRONEUSNICKNAME:	return ":<server> <code> <nick> <new_nick> :Erroneous nickname";
		case ERR_NICKNAMEINUSE:		return ":<server> <code> <nick> <new_nick> :Nickname is already in use";
		case ERR_NOTREGISTERED:		return ":<server> <code> <nick> :You have not registered";
		case ERR_NEEDMOREPARAMS:	return ":<server> <code> <nick> <command> :Not enough parameters";
		case ERR_ALREADYREGISTRED:	return ":<server> <code> <nick> :You may not re-register";
		case ERR_PASSWDMISMATCH:	return ":<server> <code> <nick> :Password incorrect";


		/// Channel operations
		case RPL_LIST:				return ":<server> <code> <nick> <channel> <users> :<topic>";
		case RPL_LISTEND:			return ":<server> <code> <nick> :End of /LIST";
		case RPL_NOTOPIC:			return ":<server> <code> <nick> <channel> :No topic is set";
		case RPL_NAMREPLY:			return ":<server> <code> <nick> <symbol> <channel> :<names>";
		case RPL_ENDOFNAMES:		return ":<server> <code> <nick> <channel> :End of /NAMES";

		case ERR_NOSUCHCHANNEL:		return ":<server> <code> <nick> <channel> :No such channel";
		case ERR_CHANNELISFULL:		return ":<server> <code> <nick> <channel> :Channel is full";
		case ERR_INVITEONLYCHAN:	return ":<server> <code> <nick> <channel> :Invite only channel";
		case ERR_BANNEDFROMCHAN:	return ":<server> <code> <nick> <channel> :Banned from channel";
		case ERR_BADCHANNELKEY:		return ":<server> <code> <nick> <channel> :Bad channel key";

		case ERR_USERNOTINCHANNEL:	return ":<server> <code> <nick> <target> <channel> :User not in channel";
		case ERR_NOTONCHANNEL:		return ":<server> <code> <nick> <channel> :You're not on that channel";
		case ERR_USERONCHANNEL:		return ":<server> <code> <nick> <target> <channel> :User already on channel";


		/// Message handling
		case ERR_NOSUCHNICK:		return ":<server> <code> <nick> :No such nickname";
		case ERR_CANNOTSENDTOCHAN:	return ":<server> <code> <nick> :Cannot send to channel";
		case ERR_NOTEXTTOSEND:		return ":<server> <code> <nick> :No text to send";
		case ERR_UNKNOWNCOMMAND:	return ":<server> <code> <nick> <command> :Unknown command";

		case RPL_WHOISUSER:			return ":<server> <code> <nick> <target> <user> <host> * :<realname>";
		case RPL_WHOISSERVER:		return ":<server> <code> <nick> <target> <server> :<server_info>";
		case RPL_WHOISPERATOR:		return ":<server> <code> <nick> <target> :is an IRC operator";
		case RPL_ENDOFWHOIS:		return ":<server> <code> <nick> <target> :End of /WHOIS list";


		default:
			return "";
	}
}
