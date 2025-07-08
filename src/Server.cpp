#include "Server.hpp"
#include "constants.hpp"
#include "Logger.hpp"
#include <termios.h>
#include "Response.hpp"

/// Static member variables

bool	Server::_terminate = false;


/// Constructors and destructors

Server::Server( const std::string port, const std::string password ) :
	_port( std::stoi(port) ),
	_password( password ),
	_serverSocket( -1 ),
	_serverStartTime( Logger::timestamp() ),
	_serverHostname( fetchHostname() ),
	_serverVersion( irc::SERVER_VERSION )
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
		std::vector<int>	removeClientFd;

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
					// TODO: Double check if the same fd has queued up messages in the sendBuffer
					if ( !receiveClientMessage( fd.fd, removeClientFd ) )
						continue ;
				}
			}
			else if ( fd.revents & POLLOUT ) // Server is ready to send message to client
			{
				/**
				 * TODO: SEND_MSG Figure out the steps
				 */
			}
			else if ( fd.revents & ( POLLERR | POLLHUP | POLLNVAL ) ) // Remove client on error or hangup
			{
				removeClientFd.push_back(fd.fd);
			}
		}

		if ( !newClients.empty() )
			_fds.insert( _fds.end(), newClients.begin(), newClients.end() );
		if ( !removeClientFd.empty() ) // Comes after adding clients as they may have already dc'd
			disconnectClients( removeClientFd );
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

void	Server::disconnectClients( std::vector<int>& remove_clients )
{
	/**
	 * NOTE: Clients may disconnect when:
	 * 		poll event flags are POLLHUP, POLLERR or POLLNVAL
	 * 		recv buffer is 0,
	 * 		recv buffer is <0 without errno EAGAIN or EWOULDBLOCK
	 * 		send return is <0 without errno EAGAIN or EWOULDBLOCK : TODO SEND_MSG
	 *
	 * 1. Remove client
	 * 2. Close it's associated fd
	 * 3. Remove associated data such as in channels // TODO: when Channels are set up
	 * 4. Log the disconnect as an event
	 */
	for ( int fd : remove_clients )
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
		// TODO: Notify channels about the disconnect
		irc::log_event("DISCONNECT", irc::LOG_SUCCESS, "client fd " + std::to_string(fd));
	}
}

// Client messaging

bool	Server::receiveClientMessage( int file_descriptor, std::vector<int>& remove_clients )
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
		if ( errno != EAGAIN && errno != EWOULDBLOCK ) // client has disconnected
		{
			remove_clients.push_back(file_descriptor);
			return (false);
		}
	}
	else if ( bytes == 0 ) // client has disconnected
	{
		remove_clients.push_back(file_descriptor);
		return (false);
	}
	else
	{
		/**
		 * TODO: SEND_MSG
		 * Process the received message.
		 * Check if the message was partial so it should be stored with Client
		 * If full message has been received (and Client buffer is empty)
		 * 	 parse the string to command structure
		 * Immediately send() the response to the client when a valid command was parsed
		 */

		Client& client = _clients[file_descriptor];

		if ( client.appendToReceiveBuffer( std::string(buffer.data(), bytes) ))
		{
			if ( client.isReceiveBufferComplete() )
			{
				// TODO: Mr. ahentton please create a command parser for us
				// Use client.exractLineFromReceive to get the full message
				// Parse the message into command structure, run the command
				// Reply with send() the result of the process with the associated reply code
			}
		}
		else // Client attempted to overflow our buffer
		{
			// Message the client with response code 417 (input line was too long)
			remove_clients.push_back(file_descriptor);
			return (false);
		}
	}
	return (true);
}


/// Exceptions

const char*	Server::InvalidClientException::what() const noexcept { return "Error: invalid client"; }
