#pragma once
#include <unordered_set>
#include "headers.hpp"

class Client
{
private:
	int								_clientFd;
	std::string						_username;
	std::string						_hostname;
	std::string						_nickname; // servername
	std::string						_realname;
	bool							_authenticated;
	std::unordered_set<std::string>	_channels;
	std::string						_receiveBuffer;
	std::string						_sendBuffer;
	sockaddr						_clientAddress;

public:
	//Constructors/Destructor
	Client();
	Client(int client_fd);
	~Client();

	// Getters
	const int								getFd				() const noexcept;
	const std::string&						getUsername			() const noexcept;
	const std::string&						getHostname			() const noexcept;
	const std::string&						getNickname			() const noexcept;
	const std::string&						getRealname			() const noexcept;
	bool									isAuthenticated		() const;
	std::unordered_set<std::string>&		getChannels			();
	sockaddr&								getClientAddress	();
	std::string&							getReceiveBuffer	();
	std::string&							getSendBuffer		();

	// Setters
	void		setClientFd				( int fd );
	void		setUsername				( std::string const &username );
	void		setHostname				( std::string const &hostname );
	void		setNickname				( std::string const &nickname );
	void		setRealname				( std::string const &realname );
	void		setClientAddress		( sockaddr address );
	void		setAuthenticated		( bool auth );

	// Buffer management
	bool		appendToReceiveBuffer	( const std::string& data );
	bool		appendToSendBuffer		( const std::string& data );
	bool		isReceiveBufferComplete	() const;
	bool		isSendBufferComplete	() const;
	std::string	extractLineFromReceive	();
	std::string extractLineFromSend		();

	// Channel management
	void		joinChannel			( const std::string& channel );
	void		leaveChannel		( const std::string& channel );
	bool		isInChannel			( const std::string& channel ) const;

protected:
	// Protected setters
	void		setReceiveBuffer	( std::string const &buffer );
	void		setSendBuffer		( std::string const &buffer );
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
