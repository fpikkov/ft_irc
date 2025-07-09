/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Command.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ahentton <ahentton@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/07 13:21:55 by ahentton          #+#    #+#             */
/*   Updated: 2025/07/09 12:26:01 by ahentton         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Command.hpp"

/*Simple helper function to ensure commands
  are always stored in uppercase format.*/
static  std::string stringToUpper(std::string cmd)
{
    for (size_t i = 0; i < cmd.length(); i++)
    {
        std::toupper(cmd[i]);
    }
    return (cmd);
}

/* joinTrail will pick up the token where ':' was found,
   and join it with the rest of the stringstream, which is picked up with std::getline.
   if token is ":hello", and trail is "there friends", ":hello there friends" is returned.*/

static  std::string joinTrail(std::string token, std::istringstream &iss)
{
	std::string trail;

	std::getline(iss, trail);

	return token + trail;
}

/* msgToCmd uses istringstream to tokenize the message.
   The function recognizes the token as either prefix, command, or parameter.
   If prefix is missing, first token is always the command portion of the message.
   Command is always stored in uppercase, hence the stringToupper helper function.
   If a trailing parameter is found, joinTrail joins rest of the stringstream into a single parameter.
   Thus concluding the process and returning the finished cmd structure*/

Command msgToCmd(std::string message)
{
	Command				cmd;
	std::istringstream	iss(message);
	std::string			token;
	size_t				token_counter = 0;

    while (iss >> token)
    {
        if (!token.empty() && token_counter == 0)
        {
            if (token[0] == ':')
                cmd.prefix = token;
            else
                cmd.command = stringToUpper(token);
        }
        else if (!token.empty() && token_counter == 1)
        {
            if (!cmd.prefix.empty())
                cmd.command = stringToUpper(token);
            else
                cmd.params.push_back(token);
        }
        else if (!token.empty())
        {
            if (token[0] == ':')
            {
                cmd.params.push_back(joinTrail(token, iss));
                return cmd;
            }
            else
                cmd.params.push_back(token);
        }
        token_counter++;
    }
    return cmd;
}
