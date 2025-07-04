#include "Client.hpp"

// Constructors/Destructor

Client::Client	() : _clientFd(-1), _authenticated(false)
{
	_clientAddress = {};
}
Client::Client	( int client_fd ) : _clientFd( client_fd ), _authenticated(false)
{
	_clientAddress = {};
}
Client::~Client	() {}


// Getters

int										Client::getFd				() const	{ return _clientFd; }
const std::string&						Client::getUsername			() const	{ return _username; }
const std::string&						Client::getHostname			() const	{ return _hostname; }
const std::string&						Client::getNickname			() const	{ return _nickname; }
const std::string&						Client::getRealname			() const	{ return _realname; }
std::string&							Client::getReceiveBuffer	()			{ return _receiveBuffer; }
std::string&							Client::getSendBuffer		()			{ return _sendBuffer; }
sockaddr&								Client::getClientAddress	()			{ return _clientAddress; }
bool									Client::isAuthenticated		() const	{ return _authenticated; }
std::unordered_set<std::string>&		Client::getChannels			()			{ return _channels; }


// Setters

void	Client::setClientFd			( int fd )							{ _clientFd = fd; }
void	Client::setUsername			( const std::string& username )		{ _username = username; }
void	Client::setHostname			( const std::string& hostname )		{ _hostname = hostname; }
void	Client::setNickname			( const std::string& nickname )		{ _nickname = nickname; }
void	Client::setRealname			( const std::string& realname )		{ _realname = realname; }
void	Client::setReceiveBuffer	( const std::string& buffer )		{ _receiveBuffer = buffer; }
void	Client::setSendBuffer		( const std::string& buffer )		{ _sendBuffer = buffer; }
void	Client::setClientAddress	( sockaddr address )				{ _clientAddress = address; }
void	Client::setAuthenticated	(bool auth)							{ _authenticated = auth; }


// Buffer management
void	Client::appendToBuffer(const std::string& data) { _buffer += data; }

// Checks if there is at least one complete IRC command in the buffer
// Returns true if \r\n is found in the buffer, false otherwise.
bool	Client::hasCompleteLine() const
{
	return _buffer.find("\r\n") != std::string::npos;
}

// Extracting and returning one complete IRC command (without \r\n) from the buffer.
std::string Client::extractLine()
{
	size_t pos = _buffer.find("\r\n");
	if (pos == std::string::npos)
		return "";
	std::string line = _buffer.substr(0, pos);
	_buffer.erase(0, pos + 2);
	return line;
}

// Adds a channel to the set of channels the client has joined.
// No duplicates are possible due to unordered_set.
void	Client::joinChannel(const std::string& channel)
{
	_channels.insert(channel);
}

void	Client::leaveChannel(const std::string& channel)
{
	_channels.erase(channel);
}

// find(channel) returns an iterator to the channel if present, or channels.end() if not.
// If not end(), the client is in the channel.
bool	Client::isInChannel(const std::string& channel) const
{
	return _channels.find(channel) != _channels.end();
}

