#include "Client.hpp"
#include "constants.hpp"

// Type definitions
using steady_clock	= std::chrono::steady_clock;
using time_point	= std::chrono::steady_clock::time_point;

// Constructors/Destructor

Client::Client() :
	_clientFd(-1),
	_authenticated(false),
	_clientAddress({}),
	_passwordAttempts(0),
	_passValidated(false),
	_active(false),
	_pollout(false)
{}

Client::Client( int client_fd ) :
	_clientFd( client_fd ),
	_authenticated(false),
	_clientAddress({}),
	_passwordAttempts(0),
	_passValidated(false),
	_active(true),
	_pollout(false),
	_connectionTime(steady_clock::now()),
	_lastActivity(steady_clock::now()),
	_pingPending(false)
{}

Client::~Client() {}


// Getters

int									Client::getFd				() const noexcept	{ return _clientFd; }
const std::string&					Client::getUsername			() const noexcept	{ return _username; }
const std::string&					Client::getHostname			() const noexcept	{ return _hostname; }
const std::string&					Client::getServername		() const noexcept	{ return _servername; }
const std::string&					Client::getNickname			() const noexcept	{ return _nickname; }
const std::string&					Client::getRealname			() const noexcept	{ return _realname; }
const std::string&					Client::getReceiveBuffer	() const noexcept	{ return _receiveBuffer; }
const std::string&					Client::getSendBuffer		() const noexcept	{ return _sendBuffer; }
const std::string&					Client::getIpAddress		() const noexcept	{ return _ipAddress; }
sockaddr&							Client::getClientAddress	()					{ return _clientAddress; }
bool								Client::isAuthenticated		() const			{ return _authenticated; }
std::unordered_set<std::string>&	Client::getChannels			()					{ return _channels; }
int									Client::getPasswordAttempts	() const noexcept	{ return _passwordAttempts; }
bool								Client::getPassValidated	() const noexcept	{ return _passValidated; }
bool								Client::getActive			() const noexcept	{ return _active; }
bool								Client::getPollout			() const noexcept	{ return _pollout; }
const time_point&					Client::getConnectionTime	() const noexcept	{ return _connectionTime; }
const time_point&					Client::getLastActivity		() const noexcept	{ return _lastActivity; }
const time_point&					Client::getLastPing			() const noexcept	{ return _lastPing; }
bool								Client::getPingPending		() const noexcept	{ return _pingPending; }


// Setters

void	Client::setClientFd			( int fd )							{ _clientFd = fd; }
void	Client::setUsername			( const std::string& username )		{ _username = username; }
void	Client::setHostname			( const std::string& hostname )		{ _hostname = hostname; }
void	Client::setServername		( const std::string& servername )	{ _servername = servername; }
void	Client::setNickname			( const std::string& nickname )		{ _nickname = nickname; }
void	Client::setRealname			( const std::string& realname )		{ _realname = realname; }
void	Client::setIpAddress		( const std::string& address )		{ _ipAddress = address; }
void	Client::setClientAddress	( sockaddr address )				{ _clientAddress = address; }
void	Client::setAuthenticated	( bool auth )						{ _authenticated = auth; }
void	Client::setReceiveBuffer	( const std::string& buffer )		{ _receiveBuffer = buffer; }
void	Client::setSendBuffer		( const std::string& buffer )		{ _sendBuffer = buffer; }
void	Client::setPasswordAttempts	( int attempts )					{ _passwordAttempts = attempts; }
void	Client::setPassValidated	( bool valid )						{ _passValidated = valid; }
void	Client::setActive			( bool active )						{ _active = active; }
void	Client::setPollout			( bool required )					{ _pollout = required; }
void	Client::setConnectionTime	( const time_point& time )			{ _connectionTime = time; }
void	Client::setLastActivity		( const time_point& time )			{ _lastActivity = time; }
void	Client::setLastPing			( const time_point& time )			{ _lastPing = time; }
void	Client::setPingPending		( bool pending )					{ _pingPending = pending; }


// Buffer management

/**
 * @brief Appends incomplete data to the client's receiving buffer.
 * Will check that the buffer limit is never exceeded.
 * @return true on successful operation, otherwise false (client should disconnect)
 */
bool	Client::appendToReceiveBuffer(const std::string& data)
{
	if ( _receiveBuffer.length() + data.length() > irc::MAX_CLIENT_BUFFER_SIZE )
	{
		irc::log_event( "PROTOCOL VIOLATION", irc::LOG_FAIL, "exceeded maximum buffer length limit" );
		return false;
	}
	_receiveBuffer += data;
	return true;
}
bool	Client::appendToSendBuffer(const std::string& data)
{
	if ( _sendBuffer.length() + data.length() > irc::MAX_CLIENT_BUFFER_SIZE )
	{
		irc::log_event( "PROTOCOL VIOLATION", irc::LOG_FAIL, "exceeded maximum buffer length limit" );
		return false;
	}
	_sendBuffer += data;
	return true;
}

void	Client::clearReceiveBuffer		()			{ _receiveBuffer.clear(); }
void	Client::clearSendBuffer			()			{ _sendBuffer.clear(); }

/**
 * @brief Checks if the receive buffer contains a complete message
 * @return true if \\r\\n is found in the buffer, false otherwise
 */
bool	Client::isReceiveBufferComplete	() const	{ return _receiveBuffer.find("\r\n") != std::string::npos; }
bool	Client::isSendBufferComplete	() const	{ return _sendBuffer.find("\r\n") != std::string::npos; }

/**
 * @brief Extracting and returning one complete IRC command (without \\r\\n) from the receive buffer.
 * Also clears up the receive buffer if a complete message was found.
 *
 * @return Complete message without \\r\\n when found. If no message or incomplete message then empty string.
 */
std::string Client::extractLineFromReceive()
{
	size_t pos = _receiveBuffer.find("\r\n");
	if (pos == std::string::npos)
		return "";
	std::string line = _receiveBuffer.substr(0, pos);
	_receiveBuffer.erase(0, pos + 2);
	return line;
}
std::string Client::extractLineFromSend()
{
	size_t pos = _sendBuffer.find("\r\n");
	if (pos == std::string::npos)
		return "";
	std::string line = _sendBuffer.substr(0, pos);
	_sendBuffer.erase(0, pos + 2);
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

// Password authentication
void	Client::incrementPassAttempts() { ++_passwordAttempts; }

// Timeout checks


bool	Client::hasRegistrationExpired()
{
	if ( _authenticated )
		return false;

	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>( steady_clock::now() - _connectionTime );
	return elapsed.count() >= irc::CLIENT_REGISTRATION_TIMEOUT;
}

bool	Client::hasPingExpired()
{
	if ( !_pingPending )
		return false;

	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>( steady_clock::now() - _lastPing );
	return elapsed.count() >= irc::CLIENT_PING_TIMEOUT;
}

bool	Client::needsPing()
{
	if ( !_authenticated || _pingPending )
		return false;

	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>( steady_clock::now() - _lastActivity );
	return elapsed.count() >= irc::CLIENT_PING_INTERVAL;
}

void	Client::updateConnectionTime()	{ _connectionTime = steady_clock::now(); }
void	Client::updateLastActivity()	{ _lastActivity = steady_clock::now(); }
void	Client::updateLastPing()		{ _lastPing = steady_clock::now(); }
