#include "Server.hpp"
#include "constants.hpp"

/// Static member variables

bool	Server::_terminate = false;


/// Constructors and destructors

Server::Server( const std::string port, const std::string password ) :
	_port( std::stoi(port) ),
	_password( password ),
	_serverSocket(-1)
{
	signal(SIGINT, Server::signalHandler);
	signal(SIGQUIT, Server::signalHandler);

	((sockaddr_in *)&_serverAddress)->sin_family = AF_INET;
	((sockaddr_in *)&_serverAddress)->sin_addr.s_addr = INADDR_ANY;
	((sockaddr_in *)&_serverAddress)->sin_port = htons( _port );
}

Server::~Server()
{
	auto it = _fds.begin();
	auto ite = _fds.end();

	for ( ; it != ite; ++it )
	{
		close((*it).fd);
		it = _fds.erase(it);
	}
}


/// Member functions

auto Server::serverSetup() -> void
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
}

auto Server::serverLoop() -> void
{
	while ( !_terminate )
	{
		if ( poll( (pollfd *)&_fds, _fds.size(), -1 ) )
		{
			if ( errno == EINTR ) // Signal was caught during poll
				break ;
			// TODO: replace with custom logging logic and return
			throw (std::runtime_error("Error: poll failure"));
		}

		std::vector<pollfd>	newClients;
		for ( auto& fd : _fds )
		{
			if ( fd.revents & POLLIN )
			{
				if ( fd.fd == _serverSocket ) // Accept new connection
				{
					/**
					 * 1. Accept the connection
					 * 2. Create pollfd from the associated client socket
					 * 3. Set events to POLLIN (and potentially others at the same time)
					 * 4. Create new Client class from the socket and store it in map
					 * 5. Store pollfd in newClients
					 * 6. Log the event
					 */
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
			else if ( fd.revents & ( POLLERR | POLLHUP | POLLNVAL ) ) // Remove client on error  or hangup
			{
				/**
				 * 1. Remove client
				 * 2. Close it's associated fd
				 * 3. Remove associated data such as in channels
				 * 4. Log the disconnect as an event
				 */
			}
		}

		if ( !newClients.empty() )
			_fds.insert( _fds.end(), newClients.begin(), newClients.end() );
	}
}


/// Signal handling

auto Server::signalHandler( int signum ) -> void
{
	if ( signum == SIGQUIT || signum == SIGINT )
		Server::_terminate = true;
}
