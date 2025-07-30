#pragma once

#include "headers.hpp"
#include <unordered_map>

class Server;
class Channel;
class Client;

class Response
{
	public:
		using string_map = std::unordered_map<std::string, std::string>;

	private:
		static std::string	_date;
		static std::string	_server;
		static std::string	_version;
		static std::string	_isupport;

		Response() = delete;

		static std::string	formatCode					( int code );
		static std::string	getResponseTemplate			( int code );
		static std::string	getCommandTemplate			( const std::string& command );
		static std::string	findAndReplacePlaceholders	( const std::string& template_string, const string_map& placeholders );

	public:
		/// Function for sending messages directly to the client
		static void	sendMessage							( Client& client, const std::string& message );

		/// Static member variable setters
		static void	setServerDate						( const std::string& date );
		static void	setServerName						( const std::string& name );
		static void	setServerVersion					( const std::string& version );
		static void	setIsupport							( const std::string& message );

		/// Functions for sending messages
		static void	sendResponseCode					( int code, Client& client, const string_map& placeholders );
		static void	sendResponseCommand					( const std::string& command, Client& source, Client& target, const string_map& placeholders );
		static void	sendPartialResponse					( Client& client );

		static void	sendServerNotice					( Client& client, const std::string& notice );
		static void	sendServerError						( Client& target, const std::string& ipAddress, const std::string& reason );
		static void	sendPing							( Client& target, const std::string& token );
		static void	sendPong							( Client& target, const std::string& token );

		/// Welcome the user
		static void	sendWelcome							( Client& client );


		/// Response codes
		/**
		 * These are standard replies a client always receives after connecting to the server.
		 * The replies provide client essential server information, and confirms the connection was succesful.
		 * Consider reply 005 optional, but nice to have. It displays the server qualities, supported commands etc.
		 */
		/// Connection registration
		static constexpr int RPL_WELCOME = 001;
		static constexpr int RPL_YOURHOST = 002;
		static constexpr int RPL_CREATED = 003;
		static constexpr int RPL_MYINFO = 004;
		static constexpr int RPL_ISUPPORT = 005;

		static constexpr int ERR_NONICKNAMEGIVEN = 431;
		static constexpr int ERR_ERRONEUSNICKNAME = 432;
		static constexpr int ERR_NICKNAMEINUSE = 433;
		static constexpr int ERR_NOTREGISTERED = 451;
		static constexpr int ERR_NEEDMOREPARAMS = 461;
		static constexpr int ERR_ALREADYREGISTERED = 462;
		static constexpr int ERR_PASSWDMISMATCH = 464;
		static constexpr int ERR_TOOMANYCHANNELS = 405;


		/// Channel operations
		static constexpr int RPL_LIST = 322;
		static constexpr int RPL_LISTEND = 323;
		static constexpr int RPL_NOTOPIC = 331;
		static constexpr int RPL_TOPIC = 332;
		static constexpr int RPL_NAMREPLY = 353;
		static constexpr int RPL_ENDOFNAMES = 366;

		static constexpr int RPL_INVITING = 341;
		static constexpr int RPL_CHANNELMODEIS = 324;

		static constexpr int ERR_NOSUCHCHANNEL = 403;
		static constexpr int ERR_CHANNELISFULL = 471;
		static constexpr int ERR_INVITEONLYCHAN = 473;
		static constexpr int ERR_BANNEDFROMCHAN = 474;
		static constexpr int ERR_BADCHANNELKEY = 475;

		static constexpr int ERR_USERNOTINCHANNEL = 441;
		static constexpr int ERR_NOTONCHANNEL = 442;
		static constexpr int ERR_USERONCHANNEL = 443;

		/// Channel operators
		static constexpr int ERR_CHANOPRIVSNEEDED = 482;

		/// Message handling
		static constexpr int ERR_NOSUCHNICK = 401;
		static constexpr int ERR_CANNOTSENDTOCHAN = 404;
		static constexpr int ERR_NOTEXTTOSEND = 412;
		static constexpr int ERR_INPUTTOOLONG = 417;
		static constexpr int ERR_UNKNOWNCOMMAND = 421;


		/// User information
		static constexpr int RPL_WHOISUSER = 311;
		static constexpr int RPL_WHOISSERVER = 312;
		static constexpr int RPL_WHOISOPERATOR = 313;
		static constexpr int RPL_ENDOFWHOIS = 318;


		/// Message of the day
		static constexpr int RPL_MOTDSTART = 375;
		static constexpr int RPL_MOTD = 372;
		static constexpr int RPL_ENDOFMOTD = 376;

		static constexpr int ERR_NOMOTD = 422;


		/// Disabled features
		static constexpr int ERR_SUMMONDISABLED = 445;
		static constexpr int ERR_USERSDISABLED = 446;

};
