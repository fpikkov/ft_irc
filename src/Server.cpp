#include "Server.hpp"
#include "Client.hpp"
#include "constants.hpp"
#include "Logger.hpp"
#include <termios.h>
#include "Response.hpp"
#include "Command.hpp"
#include "Channels.hpp"

/// Static member variables

bool	Server::_terminate = false;
bool	Server::_disconnectEvent = false;
bool	Server::_polloutEvent = false;


/// Constructors and destructors

Server::Server( const std::string port, const std::string password ) :
	_port( std::stoi(port) ),
	_password( password ),
	_serverSocket( -1 ),
	_serverStartTime( Logger::timestamp() ),
	_serverHostname( fetchHostname() ),
	_serverVersion( irc::SERVER_VERSION ),
	_commandHandler(*this)
{
	signalSetup( true );

	((sockaddr_in *)&_serverAddress)->sin_family = AF_INET;
	((sockaddr_in *)&_serverAddress)->sin_addr.s_addr = INADDR_ANY;
	((sockaddr_in *)&_serverAddress)->sin_port = htons( _port );

	Response::setServerDate		( _serverStartTime );
	Response::setServerName		( _serverHostname );
	Response::setServerVersion	( _serverVersion);
}

Server::~Server()
{
	for ( const auto& fd : _fds )
	{
		if ( fd.fd >= 0 )
			close( fd.fd );
	}

	if ( !_fds.empty() )
		_fds.clear();
	if ( !_clients.empty() )
		_clients.clear();

	signalSetup( false );
}

/// Getters

const std::string&	Server::getServerStartTime	() const { return (_serverStartTime); }
const std::string&	Server::getServerHostname	() const { return (_serverHostname); }
const std::string&	Server::getServerVersion	() const { return (_serverVersion); }


/// Setters

void	Server::setDisconnectEvent	( bool event ) { _disconnectEvent = event; }
void	Server::setPolloutEvent		( bool event ) { _polloutEvent = event; }


/// Member functions

void	Server::serverSetup()
{
	_serverSocket = socket( AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP );
	if ( _serverSocket < 0 )
		throw ( std::runtime_error("Error: failed to create server socket.") );

	int option = 1;
	if ( setsockopt( _serverSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof( option ) ) < 0 )
		throw ( std::runtime_error("Error: failed to set socket options.") );

	if ( setsockopt( _serverSocket, SOL_SOCKET, SO_REUSEPORT, &option, sizeof( option ) ) < 0 )
		throw ( std::runtime_error("Error: failed to set socket options.") );

	if ( bind( _serverSocket, &_serverAddress, sizeof( _serverAddress )) < 0 )
		throw ( std::runtime_error("Error: failed to bind server socket.") );

	if ( listen( _serverSocket, irc::MAX_CONNECTION_REQUESTS ) < 0 )
		throw ( std::runtime_error("Error: failed to listen on port: " + std::to_string(_port)) );

	pollfd	serverPoll;

	serverPoll.fd = _serverSocket;
	serverPoll.events = POLLIN;
	serverPoll.revents = 0;

	_fds.insert( _fds.cbegin(), serverPoll );

	irc::log_event("SERVER", irc::LOG_SUCCESS, "running on port " + std::to_string(_port));
}

void	Server::serverLoop()
{
	while ( !_terminate )
	{
		if ( poll( _fds.data(), _fds.size(), -1 ) < 0 )
		{
			if ( errno == EINTR ) // Signal was caught during poll
			{
				irc::log_event("SERVER", irc::LOG_DEBUG, "shutting down");
				break ;
			}
		}

		std::vector<pollfd>	newClients;

		for ( auto& fd : _fds )
		{
			if ( fd.revents & POLLIN )
			{
				if ( fd.fd == _serverSocket ) // Accept new connection
				{
					if ( !acceptClientConnection( newClients ) )
						continue ;
				}
				else // Client is sending a new message
				{
					if ( !receiveClientMessage( fd.fd ) )
						continue ;
				}
			}
			else if ( fd.revents & POLLOUT ) // Server is ready to send message to client
			{
				Response::sendPartialResponse( _clients[fd.fd] );
				if ( _clients[fd.fd].getPollout() == false )
					fd.events &= ~POLLOUT;
			}
			else if ( fd.revents & ( POLLERR | POLLHUP | POLLNVAL ) ) // Remove client on error or hangup
			{
				_clients[fd.fd].setActive(false);
				_disconnectEvent = true;
			}
		}

		if ( !newClients.empty() )
			_fds.insert( _fds.end(), newClients.begin(), newClients.end() );
		if ( _polloutEvent )
			setClientsToPollout();
		if ( _disconnectEvent ) // Comes after adding clients as they may have already dc'd
			disconnectClients();
	}
}


/// Signal handling

void	Server::signalSetup( bool start ) noexcept
{
	static termios	new_terminal;
	static termios	old_terminal;

	if ( start )
	{
		signal(SIGINT, Server::signalHandler);
		signal(SIGQUIT, Server::signalHandler);

		tcgetattr(STDIN_FILENO, &old_terminal);
		new_terminal = old_terminal;
		new_terminal.c_lflag &= ~ECHOCTL;
		old_terminal.c_lflag |= ECHOCTL;
		tcsetattr(STDIN_FILENO, TCSANOW, &new_terminal);
	}
	else
	{
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);

		tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal);
	}
}

void	Server::signalHandler( int signum )
{
	if ( signum == SIGQUIT || signum == SIGINT )
		Server::_terminate = true;
}


/// Hostname

std::string	Server::fetchHostname()
{
	std::vector<char>	hostname( irc::MAX_HOSTNAME_LENGTH );

	if ( gethostname(hostname.data(), irc::MAX_HOSTNAME_LENGTH - 1) == 0 )
		return (std::string(hostname.data()));
	return (irc::DEFAULT_HOSTNAME);
}


/// Client handling

/**
 * @brief Acccepts a new client connection and stores it if successful.
 *
 * @param[in] new_clients the temporary vector where to store new client data
 * @return true on success, otherwise false
 */
bool	Server::acceptClientConnection( std::vector<pollfd>& new_clients )
{
	/**
	 * 1. Accept the connection
	 * 2. Create pollfd from the associated client socket
	 * 3. Set events to POLLIN
	 * 4. Create new Client class from the socket and store it in map
	 * 5. Store pollfd in newClients
	 * 6. Log the event
	 */
	Client		newClient;
	sockaddr	clientAddress = {};
	socklen_t	clientAddrLen = sizeof( clientAddress );

	int	newClientSocket = accept( _serverSocket, &clientAddress, &clientAddrLen );
	if ( newClientSocket < 0 )
	{
		irc::log_event("CONNECTION", irc::LOG_FAIL, "accept failed");
		return ( false ) ;
	}

	newClient.setClientFd( newClientSocket );
	newClient.setClientAddress( clientAddress );

	Server::fetchClientIp( newClient );

	pollfd	clientPoll;
	clientPoll.fd = newClientSocket;
	clientPoll.events = POLLIN;
	clientPoll.revents = 0;
	new_clients.push_back(clientPoll);

	_clients[newClientSocket] = newClient;

	irc::log_event("CONNECTION", irc::LOG_SUCCESS, "client connected from " + newClient.getIpAddress());

	return ( true );
}

/**
 * @brief Disconnects all clients marked as inactive.
 * Closes the associated file descriptor and removes the associated pollfd.
 *
 * NOTE: This will not alert all channel members about the disconnect
 */
void	Server::disconnectClients()
{
	std::vector<int> clientsToRemove;

	for ( const auto& [fd, client] : _clients )
	{
		if ( client.getActive() == false )
			clientsToRemove.push_back( client.getFd() );
	}

	for ( int fd : clientsToRemove )
	{
		irc::log_event("DISCONNECT", irc::LOG_INFO, _clients[fd].getNickname() + "@" + _clients[fd].getIpAddress());

		close( fd );
		_clients.erase( fd );

		for ( auto it = _fds.begin(); it != _fds.end(); ++it )
		{
			if ( it->fd == fd )
			{
				_fds.erase(it);
				break ;
			}
		}

		for ( auto it = _channels.begin(); it != _channels.end(); )
		{
			if ( it->isMember(fd) )
				it->removeMember(fd);
			if ( it->isOperator(fd) )
				it->removeOperator(fd);

			if ( it->isEmpty() )
				it = _channels.erase(it);
			else
				++it;
		}
	}

	_disconnectEvent = false;
}

/// Client messaging

bool	Server::receiveClientMessage( int file_descriptor )
{
	/**
	 * 1. Receive message from client
	 * 2. Parse the message { <prefix> <command> <parameters> < : trailing_parameters > }
	 * 3. Check validity of client
	 * 4a Set Client properties
	 * 4b Execute command
	 * 5. Log the event
	 */
	std::vector<char>	buffer( irc::MAX_IRC_MESSAGE_LENGTH + 1 );

	ssize_t bytes = recv( file_descriptor, buffer.data(), irc::MAX_IRC_MESSAGE_LENGTH, 0 );

	if ( bytes < 0 )
	{
		if ( errno != EAGAIN && errno != EWOULDBLOCK )
		{
			_clients[file_descriptor].setActive(false);
			_disconnectEvent = true;
			return (false);
		}
	}
	else if ( bytes == 0 )
	{
		_clients[file_descriptor].setActive(false);
		_disconnectEvent = true;
		return (false);
	}
	else
	{
		Client& client = _clients[file_descriptor];

		if ( client.appendToReceiveBuffer( std::string(buffer.data(), bytes) ))
		{
			if ( client.isReceiveBufferComplete() )
			{
				std::string	message(client.extractLineFromReceive());
				//create toupperstr func
				Command	cmd = msgToCmd(message);
				executeCommand(client, cmd);
				//confirm_command(server->client) with send()
				// TODO: Mr. ahentton please create a command parser for us. DONE
				// Use client.exractLineFromReceive to get the full message. DONE
				// Parse the message into command structure, run the command. DONE
				// Execute the command via server::executeCommand
				// Reply with send() the result of the process with the associated reply code.
			}
		}
		else // Client attempted to overflow our buffer
		{
			Response::sendResponseCode( Response::ERR_INPUTTOOLONG, client, {} );
			client.setActive(false);
			_disconnectEvent = true;
			return (false);
		}
	}
	return (true);
}


/// Helper functions

/**
 * @brief Adds POLLOUT flag to the pollfd events for clients with buffered outgoing messages.
 */
void	Server::setClientsToPollout()
{
	for ( auto& [fd, client] : _clients )
	{
		if ( client.getPollout() )
		{
			for ( auto& element : _fds )
			{
				if ( element.fd == static_cast<int>(fd) )
				{
					element.events |= POLLOUT;
					client.setPollout(false);
					break ;
				}
			}
		}
	}

	_polloutEvent = false;
}

void	Server::executeCommand( Client& client, Command& cmd )
{
	this->_commandHandler.handleCommand(client, cmd);
}

Channel*	Server::findChannel( const std::string& channelName )
{
	for (auto& channel : _channels)
	{
		if (channel.getName() == channelName)
			return &channel;
	}
	return nullptr;
}

Client*	Server::findUser( const std::string& nickName )
{
	for (auto& client : _clients)
	{
		if (client.second.getNickname() == nickName)
			return &client.second;
	}
	return nullptr;
}

/**
 * @brief Creates a new channel and adds it to the channels vector.
 * Converts channelName to lowercase as an extra security measure.
 *
 * @param channelName Name of the new channel to be created.
 */
void	Server::addChannel( const std::string channelName )
{
	std::string lowercaseName = channelName;
	std::transform(lowercaseName.begin(), lowercaseName.end(), lowercaseName.begin(), ::tolower);

	Channel	channel(channelName);
	_channels.push_back(channel);
}

/**
 * @brief Attempts to look up the new client's ip address
 *
 * @param client Whose ip address to fetch and store.
 */
void	Server::fetchClientIp( Client& client )
{
	char			ip_buffer[INET6_ADDRSTRLEN];
	std::string		ip;
	sockaddr*		addr;

	addr = &client.getClientAddress();

	if ( irc::EXTENDED_DEBUG_LOGGING && irc::ANNOUNCE_CLIENT_LOOKUP )
		irc::log_event("CONNECTION", irc::LOG_DEBUG, "looking up client hostname");
	if ( irc::ANNOUNCE_CLIENT_LOOKUP )
		Response::sendServerNotice( client, irc::CLIENT_HOSTNAME_MESSAGE );

	if ( addr->sa_family == AF_INET )
	{
		sockaddr_in* addr_in = (sockaddr_in*)addr;
		if ( inet_ntop( AF_INET, &addr_in->sin_addr, ip_buffer, INET_ADDRSTRLEN ) != nullptr )
			ip = std::string(ip_buffer);
	}
	else if ( addr->sa_family == AF_INET6 )
	{
		sockaddr_in6* addr_in6 = (sockaddr_in6*)addr;
		if ( inet_ntop( AF_INET6, &addr_in6->sin6_addr, ip_buffer, INET6_ADDRSTRLEN ) != nullptr )
			ip = std::string(ip_buffer);
	}

	if ( irc::ANNOUNCE_CLIENT_LOOKUP )
	{
		if ( ip.empty() )
		{
			irc::log_event("CONNECTION", irc::LOG_FAIL, "failed to resolve IP address");
			Response::sendServerNotice( client, irc::CLIENT_HOSTNAME_FAILURE_MESSAGE );
		}
		else
		{
			irc::log_event("CONNECTION", irc::LOG_SUCCESS, "resolved IP: " + ip);
			Response::sendServerNotice( client, irc::CLIENT_HOSTNAME_SUCCESS_MESSAGE );
		}
	}

	client.setIpAddress( ip );
}


void	Server::removeChannel( const std::string& channelName)
{
	for (auto it = _channels.begin(); it != _channels.end(); it++)
	{
		if (it->getName() == channelName)
		{
			_channels.erase(it);
			return ;
		}
	}
}
/// Exceptions

const char*	Server::InvalidClientException::what() const noexcept { return "Error: invalid client"; }

const	std::unordered_map<unsigned, Client>&	Server::getClients() const	{ return _clients; }
const	std::vector<Channel>					Server::getChannels() const	{ return _channels; }
const	std::string&							Server::getPassword() const	{ return _password; }
