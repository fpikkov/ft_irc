#include "Server.hpp"
#include "constants.hpp"
#include "Logger.hpp"
#include <termios.h>

/// Static member variables

bool	Server::_terminate = false;


/// Constructors and destructors

Server::Server( const std::string port, const std::string password ) :
	_port( std::stoi(port) ),
	_password( password ),
	_serverSocket(-1)
{
	signalSetup( true );

	((sockaddr_in *)&_serverAddress)->sin_family = AF_INET;
	((sockaddr_in *)&_serverAddress)->sin_addr.s_addr = INADDR_ANY;
	((sockaddr_in *)&_serverAddress)->sin_port = htons( _port );
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

	if ( listen( _serverSocket, BACKLOG ) < 0 )
		throw ( std::runtime_error("Error: failed to listen on port: " + std::to_string(_port)) );

	pollfd	serverPoll;

	serverPoll.fd = _serverSocket;
	serverPoll.events = POLLIN;
	serverPoll.revents = 0;

	_fds.insert( _fds.cbegin(), serverPoll );

	Logger::instance().log("SERVER LAUNCH", LOG_SUCCESS, "running on port " + std::to_string(_port));
}

void	Server::serverLoop()
{
	while ( !_terminate )
	{
		if ( poll( _fds.data(), _fds.size(), -1 ) < 0 )
		{
			if ( errno == EINTR ) // Signal was caught during poll
			{
				Logger::instance().log("POLL", LOG_DEBUG, "received shutdown signal");
				break ;
			}
			Logger::instance().log("POLL", LOG_FAIL, "shutting down");
		}

		std::vector<pollfd>	newClients;
		std::vector<int>	removeClientFd;
		for ( auto& fd : _fds )
		{
			if ( fd.revents & POLLIN )
			{
				if ( fd.fd == _serverSocket ) // Accept new connection
				{
					if (!acceptClientConnection( newClients ));
						continue ;
				}
				else // Client is sending a new message
				{
					/**
					 * 1. Receive message from client
					 * 2. Parse the message { <prefix> <command> <parameters> }
					 * 3. Check validity of client
					 * 4a Set Client properties
					 * 4b Execute command
					 * 5. Log the event
					 */
				}
			}
			else if ( fd.revents & POLLOUT ) // Server is ready to send message to client
			{
				/**
				 * TODO: Figure out the steps
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
		Logger::instance().log("NEW CONNECTION", LOG_FAIL, "accept failed");
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

	Logger::instance().log("NEW CONNECTION", LOG_SUCCESS, "accepted on fd " + std::to_string(newClientSocket));
	return ( true );
}

void	Server::disconnectClients( std::vector<int>& remove_clients )
{
	/**
	 * NOTE: Clients may disconnect when:
	 * 		poll event flags are POLLHUP, POLLERR or POLLNVAL
	 * 		recv buffer is 0,
	 * 		recv buffer is <0 without errno EAGAIN or EWOULDBLOCK
	 * 		send return is <0 without errno EAGAIN or EWOULDBLOCK
	 *
	 * 1. Remove client
	 * 2. Close it's associated fd
	 * 3. Remove associated data such as in channels
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
		Logger::instance().log("DISCONNECT", LOG_SUCCESS, "client fd " + std::to_string(fd));
	}
}
