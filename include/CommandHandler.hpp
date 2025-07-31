#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <iostream>

class	Server;
class	Client;
class	Channel;
struct	Command;
struct	Mode;

class	CommandHandler
{

	private:
			Server&	_server;
			// std::function allows us to store any callable (lambda, function pointer, etc.). "using" is a type alias for handler functions
			using HandleFunction = std::function<void(Client&, const Command&)>;
			// Mapping command name with the handler function
			std::unordered_map<std::string, HandleFunction>	_handlers;

			// Registration commands
			void	handlePass		(Client&, const Command&);
			void	handleNick		(Client&, const Command&);
			void	handleUser		(Client&, const Command&);

			// Message commands
			void	handlePrivmsg	(Client&, const Command&);
			void	handleNotice	(Client&, const Command&);

			//Channel commands
			void	handleJoin		(Client&, const Command&);
			void	handlePart		(Client&, const Command&);
			void	handleKick		(Client&, const Command&);
			void	handleInvite	(Client&, const Command&);
			void	handleTopic		(Client&, const Command&);
			void	handleMode		(Client&, const Command&);

			// Rest of the commands
			void	handleQuit		(Client&, const Command&);
			void	handlePing		(Client&, const Command&);
			void	handlePong		(Client&, const Command&);

			void	handleSummon	(Client&, const Command&);
			void	handleUsers		(Client&, const Command&);
			void	handleWhois		(Client&, const Command&);
			void	handleWho		(Client&, const Command&);


			// Helper functions for handling modes
			void	handleChannelMode(Client& client, const Command& cmd, const std::string& channelName);
			void	sendChannelModeReply(Client& client, Channel* channel, const std::string& channelName);
			bool	parseChannelModes(Client& client, const Command& cmd, std::vector<Mode>& modes);
			bool	constructModeNodes( Client& client, const Command& cmd, const std::string& tokens, size_t& paramIndex, std::vector<Mode>& modes );
			void	applyChannelModes(Client& client, Channel& channel, const std::vector<Mode>& modes);

			void	handleModeInviteOnly(Channel& channel, bool adding);
			void	handleModeTopicLocked(Channel& channel, bool adding);
			void	handleModeKey(Channel& channel, bool adding, const Mode& mode);
			void	handleModeLimit(Channel& channel, bool adding, const Mode& mode);
			void	handleModeOperator(Client& client, Channel& channel, bool adding, const Mode& mode);

			// Channel broadcasting functions
			void	broadcastJoin		( Client& client, Channel& channel );
			void	broadcastPrivmsg	( Client& client, Channel& channel, const std::string& message );
			void	broadcastNotice		( Client& client, Channel& channel, const std::string& message );
			void	broadcastPart		( Client& client, Channel& channel, const std::string& message );
			void	broadcastKick		( Client& client, Client& target, Channel& channel, const std::string & message );
			void	broadcastQuit		( Client& client, const std::string& message );
			void	broadcastMode		( Client& client, Channel& channel, const std::string& modeStr);
			void	broadcastTopic		( Client& client, Channel& channel, const std::string& newTopic );

			// Authorization function
			bool	confirmAuth			( Client& client );

			// Static helper functions
			static bool			isValidNick		( const std::string& nick );
			static bool			isChannelName	( const std::string& name );
			static std::string	toLowerCase		( const std::string& s );

			// Mode related static hellper functions
			static bool			isMode			( char mode );
			static bool			isSign			( char sign );
			static bool			requiresParam	( char mode, bool adding );
			static std::string	inlineParam		( const std::string& tokens, size_t& index, std::function<bool(char)> condition );

	public:
			CommandHandler(Server& server);
			void	handleCommand(Client& client, const Command& cmd);

};
