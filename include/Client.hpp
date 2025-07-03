#pragma once
#include <unordered_set>

#include "headers.hpp"

class Client
{
private:
	int								_client_fd;
	std::string						_nickname;
	std::string						_username;
	std::string						_realname;
	std::string						_hostname;
	std::string						_buffer;
	bool							authenticated;
	std::unordered_set<std::string>	channels;


public:
	//Getters
	int		getFd() const;
	const 	std::string& getNickname() const;
	const 	std::string& getUsername() const;
	const 	std::string& getRealname() const;
	const 	std::string& getHostname() const;
	bool	isAuthenticated() const;
	bool	getOperatorStatus() const;
	const	std::unordered_set<std::string>& getChannels() const;

	//Setters
	void	setNickname(std::string const &nickname);
	void	setUsername(std::string const &username);
	void	setRealname(std::string const &realname);
	void	setHostname(std::string const &realname);
	void	setAuthenticated(bool auth);

	//Buffer management
	void	appendToBuffer(const std::string& data);
	std::string	extractLine();

	//Channel management
	void	joinChannel(const std::string& channel);
	void	leaveChannel(const std::string& channel);

	//Constructors/Destructor
	Client(int client_fd);
	~Client();
};

/*
_client_fd		Identify and communicate with the client (multi-client, non-blocking I/O)
_nickname		Unique user identity (NICK command, protocol requirement)
_username		User authentication (USER command, protocol requirement)
_realname		Full USER command support
_hostname		Advanced protocol/logging, recommended
_buffer			Handle partial/fragmented messages (TCP stream, subject test example)
authenticated	Enforce authentication before allowing actions
channels		Track channel membership (JOIN/PART, message forwarding)
 */