#pragma once

#include "headers.hpp"
#include <vector>
#include <unordered_map>
#include <chrono>
#include "CommandHandler.hpp"

class	Channel;
struct	Command;
class	Client;

class Server
{
	private:
		int										_port;
		std::string								_password;
		int										_serverSocket;
		std::unordered_map<unsigned, Client>	_clients;
		std::vector<Channel>					_channels;
		std::vector<pollfd>						_fds;
		sockaddr								_serverAddress;
		std::string								_serverStartTime;
		std::string								_serverHostname;
		const std::string						_serverVersion;
		static bool								_terminate;
		CommandHandler							_commandHandler;
		static bool								_disconnectEvent;
		static bool								_polloutEvent;
		std::chrono::steady_clock::time_point	_lastTimeoutCheck;

		Server()								= delete;
		Server( const Server& )					= delete;
		Server& operator=( const Server& )		= delete;

		void				signalSetup			( bool start ) noexcept;
		static void 		signalHandler		( int signum );
		std::string			fetchHostname		();
		static void			fetchClientIp		( Client& client );
		void				setClientsToPollout	();
		void				checkTimeouts		();

	public:
		Server( const std::string port, const std::string password );
		~Server();

		const std::string&							getServerStartTime	() const;
		const std::string&							getServerHostname	() const;
		const std::string&							getServerVersion	() const;
		const std::unordered_map<unsigned, Client>& getClients			() const;
		const std::string&							getPassword			() const;
		const std::vector<Channel>					getChannels			() const;

		static void	setDisconnectEvent	( bool event );
		static void	setPolloutEvent		( bool event );

		void		serverSetup				();
		void		serverLoop				();
		bool		acceptClientConnection	( std::vector<pollfd>& new_clients );
		bool		receiveClientMessage	( int file_descriptor );
		void		disconnectClients		();
		void		executeCommand			( Client& client, Command& cmd);
		void		broadcastShutdown		( const std::string& reason );

		void		addChannel				( const std::string channelName);
		void		removeChannel			( const std::string& channelName);
		Channel*	findChannel				( const std::string& channelName );
		Client*		findUser				( const std::string& nickName );

};

