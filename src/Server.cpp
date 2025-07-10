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

	if ( bind( _serverSocket, &_serverAddress, sizeof( _serverAddress )) < 0 )
		throw ( std::runtime_error("Error: failed to bind server socket.") );

	if ( listen( _serverSocket, irc::MAX_CONNECTION_REQUESTS ) < 0 )
		throw ( std::runtime_error("Error: failed to listen on port: " + std::to_string(_port)) );

	pollfd	serverPoll;

	serverPoll.fd = _serverSocket;
	serverPoll.events = POLLIN;
	serverPoll.revents = 0;

	_fds.insert( _fds.cbegin(), serverPoll );

	irc::log_event("SERVER LAUNCH", irc::LOG_SUCCESS, "running on port " + std::to_string(_port));
}

void	Server::serverLoop()
{
	while ( !_terminate )
	{
		if ( poll( _fds.data(), _fds.size(), -1 ) < 0 )
		{
			if ( errno == EINTR ) // Signal was caught during poll
			{
				irc::log_event("POLL", irc::LOG_DEBUG, "received shutdown signal");
				break ;
			}
			irc::log_event("POLL", irc::LOG_FAIL, "shutting down");
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
				if ( irc::EXTENDED_DEBUG_LOGGING )
					irc::log_event( "POLL", irc::LOG_DEBUG, "client has disconnected due to error" );
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
		irc::log_event("NEW CONNECTION", irc::LOG_FAIL, "accept failed");
		return ( false ) ;
	}

	newClient.setClientFd( newClientSocket );
	newClient.setClientAddress( clientAddress );

	pollfd	clientPoll;
	clientPoll.fd = newClientSocket;
	clientPoll.events = POLLIN;
	clientPoll.revents = 0;
	new_clients.push_back(clientPoll);

	_clients[newClientSocket] = newClient;

	irc::log_event("NEW CONNECTION", irc::LOG_SUCCESS, "accepted on fd " + std::to_string(newClientSocket));
	return ( true );
}

/**
 * @brief Disconnects all clients marked as inactive.
 * Closes the associated file descriptor and removes the associated pollfd.
 */
void	Server::disconnectClients()
{
	/**
	 * 1. Remove client
	 * 2. Close it's associated fd
	 * 3. Remove associated data such as in channels // TODO: when Channels are set up
	 * 4. Log the disconnect as an event
	 */

	std::vector<int> clientsToRemove;

	for ( const auto& [fd, client] : _clients )
	{
		if ( client.getActive() == false )
			clientsToRemove.push_back( client.getFd() );
	}

	for ( int fd : clientsToRemove )
	{
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
		// TODO: Notify channel users about the disconnect AND remove the client from user lists
		irc::log_event("DISCONNECT", irc::LOG_SUCCESS, "client fd " + std::to_string(fd));
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
			if ( irc::EXTENDED_DEBUG_LOGGING )
				irc::log_event( "RECV", irc::LOG_DEBUG, "client has disconnected" );
			return (false);
		}
	}
	else if ( bytes == 0 )
	{
		_clients[file_descriptor].setActive(false);
		_disconnectEvent = true;
		if ( irc::EXTENDED_DEBUG_LOGGING )
			irc::log_event( "RECV", irc::LOG_DEBUG, "client has disconnected" );
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
			// TODO: Message the client with response code 417
			_clients[file_descriptor].setActive(false);
			_disconnectEvent = true;
			if ( irc::EXTENDED_DEBUG_LOGGING )
				irc::log_event( "RECV", irc::LOG_DEBUG, "input line too long, dropping connection" );
			return (false);
		}
	}
	return (true);
}

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

					if ( irc::EXTENDED_DEBUG_LOGGING )
						irc::log_event( "POLL", irc::LOG_DEBUG, "polling out to client" );

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

Client*	Server::findUser( std::string& nickName )
{
	for (auto& client : _clients)
	{
		if (client.second.getNickname() == nickName)
			return &client.second;
	}
	return nullptr;
}

void	Server::addChannel(const std::string channelName)
{
	Channel	channel(channelName);
	_channels.push_back(channel);	
}
/// Exceptions

const char*	Server::InvalidClientException::what() const noexcept { return "Error: invalid client"; }

const	std::unordered_map<unsigned, Client>& Server::getClients() const { return _clients; }
const	std::vector<Channel> Server::getChannels() const { return _channels; }
const	std::string& Server::getPassword() const { return _password; }
