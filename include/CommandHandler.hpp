#pragma once
#include <functional>
#include <unordered_map>
#include <iostream>

class	Server;
class	Client;
struct	Command;
class	CommandHandler
{

	private:
			Server&	_server;
			// std::function allows us to store any callable (lambda, function pointer, etc.). "using" is a type alias for handler functions
			using HandleFunction = std::function<void(Client&, const Command&)>;
			// Mapping command name with the handler function
			std::unordered_map<std::string, HandleFunction> _handlers;

			// Registration commands
			void handlePass(Client&, const Command&);
			void handleNick(Client&, const Command&);
			void handleUser(Client&, const Command&);

			// Message commands
			void handlePrivmsg(Client&, const Command&);
			void handleNotice(Client&, const Command&);

			//Channel commands
			void handleJoin(Client&, const Command&);
			void handlePart(Client&, const Command&);
			void handleKick(Client&, const Command&);
			void handleInvite(Client&, const Command&);
			void handleTopic(Client&, const Command&);
			void handleMode(Client&, const Command&);

			// Rest of the commands
			void handleQuit(Client&, const Command&);
			void handlePing(Client&, const Command&);
			void handlePong(Client&, const Command&);


	public:
			CommandHandler(Server& server);
			void	handleCommand(Client& client, const Command& cmd);

};


// Minimum commands to cover
// PASS
// NICK
// USER
// QUIT
// JOIN
// PART
// PRIVMSG
// NOTICE(not sure)
// PING
