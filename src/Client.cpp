#include "Client.hpp"

//Setters

void	Client::setClientFd			( int fd )							{ _clientFd = fd; }
void	Client::setUsername			( std::string const &username )		{ _username = username; }
void	Client::setHostname			( std::string const &hostname )		{ _hostname = hostname; }
void	Client::setNickname			( std::string const &nickname )		{ _nickname = nickname; }
void	Client::setRealname			( std::string const &realname )		{ _realname = realname; }
void	Client::setReceiveBuffer	( std::string const &buffer )		{ _receiveBuffer = buffer; }
void	Client::setSendBuffer		( std::string const &buffer )		{ _sendBuffer = buffer; }
void	Client::setClientAddress	( sockaddr address )				{ _clientAddress = address; }


//Getters

int					Client::getFd				() const	{ return _clientFd; }
std::string const	&Client::getNickname		() const	{ return _nickname; }
std::string const	&Client::getUsername		() const	{ return _username; }
std::string const	&Client::getRealname		() const	{ return _realname; }
std::string			&Client::getReceiveBuffer	()			{ return _receiveBuffer; }
std::string			&Client::getSendBuffer		()			{ return _sendBuffer; }
sockaddr			&Client::getClientAddress	()			{ return _clientAddress; }


//NOTE: Constructors/Destructor

Client::Client	() : _clientFd(-1)
{
	_clientAddress = {};
}
Client::Client	( int client_fd ) : _clientFd( client_fd )
{
	_clientAddress = {};
}
Client::~Client	() {}
