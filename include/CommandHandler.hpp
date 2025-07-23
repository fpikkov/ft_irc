#pragma once
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <iostream>

class	Server;
class	Client;
class	Channel;
struct	Command;

class	CommandHandler
{

	private:
			Server&	_server;
			// std::function allows us to store any callable (lambda, function pointer, etc.). "using" is a type alias for handler functions
			using HandleFunction = std::function<void(Client&, const Command&)>;
			// Mapping command name with the handler function
			std::unordered_map<std::string, HandleFunction>	_handlers;

			// Registration commands
			void	handlePass(Client&, const Command&);
			void	handleNick(Client&, const Command&);
			void	handleUser(Client&, const Command&);

			// Message commands
			void	handlePrivmsg(Client&, const Command&);
			void	handleNotice(Client&, const Command&);

			//Channel commands
			void	handleJoin(Client&, const Command&);
			void	handlePart(Client&, const Command&);
			void	handleKick(Client&, const Command&);
			void	handleInvite(Client&, const Command&);
			void	handleTopic(Client&, const Command&);
			void	handleMode(Client&, const Command&);

			// Rest of the commands
			void	handleQuit(Client&, const Command&);
			void	handlePing(Client&, const Command&);
			void	handlePong(Client&, const Command&);


			// Helper functions for handling modes
			void	handleChannelMode(Client& client, const Command& cmd, const std::string& channelName);
			void	sendChannelModeReply(Client& client, Channel* channel, const std::string& channelName, const Command& cmd);
			void	parseAndApplyChannelModes(Client& client, Command& cmd, Channel* channel, const std::string& channelName);
			void	handleModeInviteOnly(Channel* channel, bool adding);
			void	handleModeTopicLocked(Channel* channel, bool adding);
			void	handleModeKey(Client& client, Command& cmd, Channel* channel, bool adding, size_t& paramIndex);
			void	handleModeLimit(Client& client, Command& cmd, Channel* channel, bool adding, size_t& paramIndex);
			void	handleModeOperator(Client& client, Command& cmd, Channel* channel, bool adding, size_t& paramIndex, const std::string& channelName);

			// Channel broadcasting functions
			void	broadcastJoin		( Client& client, Channel& channel );
			void	broadcastPrivmsg	( Client& client, Channel& channel, const std::string& message );
			void	broadcastNotice		( Client& client, Channel& channel, const std::string& message );
			void	broadcastPart		( Client& client, Channel& channel, const std::string& message );
			void	broadcastQuit		( Client& client, const std::string& message );
			void	broadcastMode		( Client& client, Channel& channel, const std::string& modeStr, const Command& cmd );

			// Static helper functions
			static bool			isValidNick		( const std::string& nick );
			static bool			isChannelName	( const std::string& name );
			static std::string	toLowerCase		( const std::string& s );

	public:
			CommandHandler(Server& server);
			void	handleCommand(Client& client, const Command& cmd);

};
