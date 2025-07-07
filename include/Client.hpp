/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ahentton <ahentton@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/03 10:54:22 by ahentton          #+#    #+#             */
/*   Updated: 2025/07/03 12:09:50 by ahentton         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "headers.hpp"

class Client
{
private:
	int			_client_fd;
	std::string	_nickname;
	std::string	_username;
	std::string	_realname;
	bool		is_connected;
	

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
