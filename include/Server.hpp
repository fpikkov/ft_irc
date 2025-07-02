#pragma once

#include "headers.hpp"
#include "Client.hpp"
#include <vector>
#include <map>

class Server
{
	private:
		int							_port;
		std::string					_password;
		int							_serverSocket;
	//	std::map<unsigned, Client>	_clients;
		std::vector<pollfd>			_fds;
		sockaddr					_serverAddress;
		static bool					_terminate;

		Server()							= delete;
		Server( const Server& )				= delete;
		Server& operator=( const Server& )	= delete;

		static auto signalHandler( int signum ) -> void;

	public:
		Server( const std::string port, const std::string password );
		~Server();

		auto serverSetup() -> void;
		auto serverLoop() -> void;

		class InvalidClientException: public std::exception
		{
			public:
				const char* what() const noexcept override;
		};;

};

