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
	bool							_authenticated;
	std::unordered_set<std::string>	_channels;


public:

	Client(int client_fd);

	// Getters
	int		getFd() const;
	const 	std::string& getNickname() const;
	const 	std::string& getUsername() const;
	const 	std::string& getRealname() const;
	const 	std::string& getHostname() const;
	bool	isAuthenticated() const;
	const	std::unordered_set<std::string>& getChannels() const;

	// Setters
	void	setNickname(const std::string& nickname);
	void	setUsername(const std::string& username);
	void	setRealname(const std::string& realname);
	void	setHostname(const std::string& hostname);
	void	setAuthenticated(bool auth);

	// Buffer management
	void		appendToBuffer(const std::string& data);
	bool		hasCompleteLine() const;
	std::string	extractLine();

	// Channel management
	void	joinChannel(const std::string& channel);
	void	leaveChannel(const std::string& channel);
	bool	isInChannel(const std::string& channel) const;
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