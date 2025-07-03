#include "Client.hpp"

//Setters
void    Client::setNickname(std::string const& nickname) { _nickname = nickname; }
void    Client::setUsername(std::string const& username) { _username = username; }
void    Client::setRealname(std::string const& realname) { _realname = realname; }
void    Client::setHostname(std::string const& hostname) { _hostname = hostname; }


//Getters
int	Client::getFd() const {return _client_fd;}
std::string const    &Client::getNickname() const {return _nickname;}
std::string const    &Client::getUsername() const {return _username;}
std::string const    &Client::getRealname() const {return _realname;}

Client::Client(int client_fd) : _client_fd(client_fd) {}
Client::~Client() {}
