/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ahentton <ahentton@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/08 13:17:30 by ahentton          #+#    #+#             */
/*   Updated: 2025/07/08 13:20:40 by ahentton         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CommandHandler.hpp"

CommandHandler::CommandHandler(Server &server) : _server(server)
{}

void    CommandHandler::handleCommand(Client &client, Command const &cmd)
{}