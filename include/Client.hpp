#pragma once
#include <unordered_set>
#include <chrono>
#include "headers.hpp"

class Client
{
private:
	int										_clientFd;
	std::string								_username;
	std::string								_hostname;
	std::string								_servername;
	std::string								_nickname; // servername
	std::string								_realname;
	bool									_authenticated;
	std::unordered_set<std::string>			_channels;
	std::string								_receiveBuffer;
	std::string								_sendBuffer;
	std::string								_ipAddress;
	sockaddr								_clientAddress;
	int										_passwordAttempts;
	bool									_passValidated;
	bool									_active;
	bool									_pollout;
	std::chrono::steady_clock::time_point	_connectionTime;
	std::chrono::steady_clock::time_point	_lastActivity;
	std::chrono::steady_clock::time_point	_lastPing;
	bool									_pingPending;

public:
	//Constructor/Destructor
	Client();
	~Client();

	// Getters
	
	int												getFd				() const noexcept;
	const std::string&								getUsername			() const noexcept;
	const std::string&								getHostname			() const noexcept;
	const std::string&								getServername		() const noexcept;
	const std::string&								getNickname			() const noexcept;
	const std::string&								getRealname			() const noexcept;
	bool											isAuthenticated		() const;
	std::unordered_set<std::string>&				getChannels			();
	const std::string&								getIpAddress		() const noexcept;
	sockaddr&										getClientAddress	();
	const std::string&								getReceiveBuffer	() const noexcept;
	const std::string&								getSendBuffer		() const noexcept;
	int												getPasswordAttempts	() const noexcept;
	bool											getPassValidated	() const noexcept;
	bool											getActive			() const noexcept;
	bool											getPollout			() const noexcept;
	const std::chrono::steady_clock::time_point&	getConnectionTime	() const noexcept;
	const std::chrono::steady_clock::time_point&	getLastActivity		() const noexcept;
	const std::chrono::steady_clock::time_point&	getLastPing			() const noexcept;
	bool											getPingPending		() const noexcept;

	// Setters
	void		setClientFd				( int fd );
	void		setUsername				( const std::string& username );
	void		setHostname				( const std::string& hostname );
	void		setServername			( const std::string& servername );
	void		setNickname				( const std::string& nickname );
	void		setRealname				( const std::string& realname );
	void		setIpAddress			( const std::string& address );
	void		setClientAddress		( sockaddr address );
	void		setAuthenticated		( bool auth );
	void		setPasswordAttempts		( int attempts );
	void		setPassValidated		( bool valid );
	void		setActive				( bool active );
	void		setPollout				( bool required );
	void		setConnectionTime		( const std::chrono::steady_clock::time_point& time );
	void		setLastActivity			( const std::chrono::steady_clock::time_point& time );
	void		setLastPing				( const std::chrono::steady_clock::time_point& time );
	void		setPingPending			( bool pending );

	// Buffer management
	bool		appendToReceiveBuffer	( const std::string& data );
	bool		appendToSendBuffer		( const std::string& data );
	void		clearReceiveBuffer		();
	void		clearSendBuffer			();
	bool		isReceiveBufferComplete	() const;
	bool		isSendBufferComplete	() const;
	std::string	extractLineFromReceive	();
	std::string extractLineFromSend		();

	// Channel management
	void		joinChannel			( const std::string& channel );
	void		leaveChannel		( const std::string& channel );
	bool		isInChannel			( const std::string& channel ) const;

	// Password authentication
	void		incrementPassAttempts	();

	// Timeout checks
	bool		hasRegistrationExpired	();
	bool		hasPingExpired			();
	bool		needsPing				();
	void		updateConnectionTime	();
	void		updateLastActivity		();
	void		updateLastPing			();

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
