/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ahentton <ahentton@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/08 12:50:10 by ahentton          #+#    #+#             */
/*   Updated: 2025/07/08 13:19:27 by ahentton         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Client.hpp"
#include "Command.hpp"
#include <functional>
#include <unordered_map>
#include <string>

class CommandHandler
{

    private:
            Server  &_server;
            void    handleCommand(Client &client, Command const &cmd);

    public:
            CommandHandler(Server &server);
};