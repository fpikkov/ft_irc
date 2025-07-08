#pragma once

#include "headers.hpp"
#include <vector>
#include <unordered_map>

struct Command;

class Client;

class Server
{
	private:
		int										_port;
		std::string								_password;
		int										_serverSocket;
		std::unordered_map<unsigned, Client>	_clients;
		std::vector<pollfd>						_fds;
		sockaddr								_serverAddress;
		std::string								_serverStartTime;
		std::string								_serverHostname;
		const std::string						_serverVersion;
		static bool								_terminate;
		static bool								_disconnectEvent;
		static bool								_polloutEvent;

		Server()								= delete;
		Server( const Server& )					= delete;
		Server& operator=( const Server& )		= delete;

		void		signalSetup			( bool start ) noexcept;
		static void signalHandler		( int signum );
		std::string	fetchHostname		();
		void		setClientsToPollout	();

	public:
		Server( const std::string port, const std::string password );
		~Server();

		const std::string&	getServerStartTime	() const;
		const std::string&	getServerHostname	() const;
		const std::string&	getServerVersion	() const;

		static void	setDisconnectEvent	( bool event );
		static void	setPolloutEvent		( bool event );

		void		serverSetup				();
		void		serverLoop				();
		bool		acceptClientConnection	( std::vector<pollfd>& new_clients );
		void		disconnectClients		();
		bool		receiveClientMessage	( int file_descriptor );
		[[maybe_unused]] void		executeCommand			( Client& client, const Command& cmd);

		class InvalidClientException: public std::exception
		{
			public:
				const char* what() const noexcept override;
		};

};

