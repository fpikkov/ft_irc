#include "headers.hpp"
#include "Server.hpp"
#include "macros.hpp"

auto main( int argc, char **argv ) -> int
{
	if (argc == 3)
	{
		try
		{
			Server server(argv[1], argv[2]);

			// TODO: restt of the server calls

			server.serverLoop();
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
	}
	else
	{
		PRINT("Usage: ircserv [port] [password]");
	}

	return (0);
}

/*

# GUIDE ON IRC COMMANDS

//User writes "/nick spongebob" on client.
//Server picks the command line up with recv(..., ..., ..., ...),
//with second argument being the variable where the data is stored.
//Possibly a char buffer[xxx]?

//Server receives that command line as "NICK spongebob\r\n",
//Parse this into a command that can be examinated and executed step by step.

//Possibly make this into a server::function later, for easy access

char    buffer[512];
ssize_t bytes_received = recv(client_fd, buffer, ..., ...);

//buffer now contains "NICK SPONGEBOB\r\n"
//thought for later, who sent it?

std::string input(buffer);
std::istringstream iss(input);

//this solution will convert the contents of buffer to a string
//istringstream iss constructor will separate the string by whitespace

//this wont exactly work if there is a ":" prefix in the buffer.
//this prefix can be used to signal that next argument holds spaces

//EXAMPLE: PRIVMSG #channel :Hello world!


*/
