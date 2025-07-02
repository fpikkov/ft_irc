#pragma once

#include "headers.hpp"

class Client
{
private:
	int		_client_fd;
	std::string	_nickname;
	std::string	_username;
	std::string	_realname;
public:
	//Setters
	void	setNickname(std::string const &nickname);
	void	setUsername(std::string const &username);
	void	setRealname(std::string const &realname);

	//Getters
	int			getFd() const;
	std::string const	&getNickname() const;
	std::string const	&getUsername() const;
	std::string const	&getRealname() const;

	//Constructors/Destructor
	Client(int client_fd);
	~Client();
};
