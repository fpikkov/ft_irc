#pragma once

#include "headers.hpp"

class Client
{
private:
	int				_clientFd;
	std::string		_username;
	std::string		_hostname;
	std::string		_nickname; // servername
	std::string		_realname;
	std::string		_receiveBuffer;
	std::string		_sendBuffer;
	sockaddr		_clientAddress;

public:
	//Setters
	void	setClientFd			( int fd );
	void	setUsername			( std::string const &username );
	void	setHostname			( std::string const &hostname );
	void	setNickname			( std::string const &nickname );
	void	setRealname			( std::string const &realname );
	void	setReceiveBuffer	( std::string const &buffer );
	void	setSendBuffer		( std::string const &buffer );
	void	setClientAddress	( sockaddr address );

	//Getters
	int					getFd				() const;
	std::string const	&getNickname		() const;
	std::string const	&getUsername		() const;
	std::string const	&getRealname		() const;
	std::string			&getReceiveBuffer	();
	std::string			&getSendBuffer		();
	sockaddr			&getClientAddress	();


	//Constructors/Destructor
	Client();
	Client(int client_fd);
	~Client();
};
