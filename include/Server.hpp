#pragma once

#include "headers.hpp"
#include "Client.hpp"
#include <vector>
#include <unordered_map>

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
		static bool								_terminate;

		Server()								= delete;
		Server( const Server& )					= delete;
		Server& operator=( const Server& )		= delete;

		void		signalSetup		( bool start ) noexcept;
		static void signalHandler	( int signum );

	public:
		Server( const std::string port, const std::string password );
		~Server();

		void		serverSetup				();
		void		serverLoop				();
		bool		acceptClientConnection	( std::vector<pollfd>& new_clients );
		void		disconnectClients		( std::vector<int>& remove_clients );
		bool		receiveClientMessage	( int file_descriptor, std::vector<int>& remove_clients );
		void		executeCommand			( Client &client, Command const &cmd);

		class InvalidClientException: public std::exception
		{
			public:
				const char* what() const noexcept override;
		};

};

