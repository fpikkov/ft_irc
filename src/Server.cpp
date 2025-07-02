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
	// TODO: make the signal handling work
	signal(SIGTERM, Server::signalHandler);
	signal(SIGQUIT, Server::signalHandler);

	((sockaddr_in *)&_serverAddress)->sin_family = AF_INET;
	((sockaddr_in *)&_serverAddress)->sin_addr.s_addr = INADDR_ANY;
	((sockaddr_in *)&_serverAddress)->sin_port = htons( _port );
}

// TODO: file descriptor cleanup
Server::~Server()
{

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

	/**
	 * TODO: Create pollfd for the server in the initialization so it woouldn't have to be done in the server loop

	pollfd	serverPoll;

	 */
}

auto Server::serverLoop() -> void
{
	while ( !_terminate )
	{
		// do stuff
	}

	if (_terminate)
		std::cerr << "Server closing" << std::endl;
}


/// Signal handling

auto Server::signalHandler( int signum ) -> void
{
	if ( signum == SIGQUIT || signum == SIGTERM )
		Server::_terminate = true;
}
