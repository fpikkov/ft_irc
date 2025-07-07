/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Command.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ahentton <ahentton@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/07 13:17:37 by ahentton          #+#    #+#             */
/*   Updated: 2025/07/07 14:48:47 by ahentton         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <vector>
#include <sstream>

struct  Command
{
    std::string prefix;
    std::string command;
    std::vector<std::string> params;
};

Command msgToCmd(std::string message);