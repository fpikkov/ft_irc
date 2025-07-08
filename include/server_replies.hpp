/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server_replies.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ahentton <ahentton@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/07 13:01:39 by ahentton          #+#    #+#             */
/*   Updated: 2025/07/07 13:01:53 by ahentton         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Replies 001-005.
//These are standard replies a client always receives after connecting to the server.
//The replies provide client essential server information, and confirms the connection was succesful.
//Consider reply 005 optional, but nice to have. It displays the server qualities, supported commands etc.

# define RPL_WELCOME(nickname) (":localhost 001 " + nickname + " :Welcome to the IRC, Mr/Mrs " + nickname)
# define RPL_YOURHOST(nickname, servername, version) (":localhost 002 " + nickname + " :Your host is " + servername + " , running version " + version)
# define RPL_CREATED(nickname, datetime) (":localhost 003 " + nickname + " :This server was created " + datetime)
# define RPL_MYINFO(nickname, servername, version, available_umodes, available_cmodes, cmodes_with_param) \
 (":localhost 004 " + nickname + " " + servername + " " + version + " " + available_umodes + " " + available_comes + " " + cmodes_with_param)
//# define RPL_BOUNCE/RPL_ISUPPORT, optional, could be fun to add later.

//Error replies.

# define ERR_NOSUCHNICK(nickname) (":localhost 401 " + nickname + " :No such nickname")
# define ERR_NOSUCHSERVER(nickname) (":localhost 402 " + nickname + " :No such server")
# define ERR_NOSUCHCHANNEL(nickname) (":localhost 403 " + nickname + " :No such channel")
# define ERR_CANNOTSENDTOCHAN(nickname) (":localhost 404 " + nickname + " :Cannot send to channel")
# define ERR_TOOMANYCHANNELS(nickname) (":localhost 405 " + nickname + " :You have joined too many channels")
# define ERR_WASNOSUCHNICK(nickname) (":localhost 406 " + nickname + " :There was no such nickname")
# define ERR_TOOMANYTARGETS(nickname) (":localhost 407 " + nickname + " :Duplicate recipients. No message delivered")
# define ERR_NOORIGIN(nickname) (":localhost 409 " + nickname + " :No origin specified")
# define ERR_NORECIPIENT(nickname) (":localhost 411 " + nickname + " :No recipient given")
# define ERR_NOTEXTTOSEND(nickname) (":localhost 412 " + nickname + " :No text to send")
# define ERR_NOTOPLEVEL(nickname) (":localhost 413 " + nickname + " :No toplevel domain specified")
# define ERR_WILDTOPLEVEL(nickname) (":localhost 414 " + nickname + " :Wildcard in toplevel domain")
# define ERR_UNKNOWNCOMMAND(nickname) (":localhost 421 " + nickname + " :Unknown command")
# define ERR_NOMOTD(nickname) (":localhost 422 " + nickname + " :MOTD File is missing")
# define ERR_NOADMININFO(nickname) (":localhost 423 " + nickname + " :No administrative info available")
# define ERR_FILEERROR(nickname) (":localhost 424 " + nickname + " :File error doing requested operation")
# define ERR_NONICKNAMEGIVEN(nickname) (":localhost 431 " + nickname + " :No nickname given")
# define ERR_ERRONEUSNICKNAME(nickname) (":localhost 432 " + nickname + " :Erroneous nickname")
# define ERR_NICKNAMEINUSE(nickname) (":localhost 433 " + nickname + " :Nickname is already in use")
# define ERR_USERNOTINCHANNEL(nickname) (":localhost 441 " + nickname + " :User not in channel")
# define ERR_NOTONCHANNEL(nickname) (":localhost 442 " + nickname + " :You're not on that channel")
# define ERR_USERONCHANNEL(nickname) (":localhost 443 " + nickname + " :User already on channel")
# define ERR_NOTREGISTERED(nickname) (":localhost 451 " + nickname + " :You have not registered")
# define ERR_NEEDMOREPARAMS(nickname) (":localhost 461 " + nickname + " :Not enough parameters")
# define ERR_ALREADYREGISTRED(nickname) (":localhost 462 " + nickname + " :You may not re-register")
# define ERR_PASSWDMISMATCH(nickname) (":localhost 464 " + nickname + " :Password incorrect")
# define ERR_CHANNELISFULL(nickname) (":localhost 471 " + nickname + " :Channel is full")
# define ERR_INVITEONLYCHAN(nickname) (":localhost 473 " + nickname + " :Invite only channel")
# define ERR_BANNEDFROMCHAN(nickname) (":localhost 474 " + nickname + " :Banned from channel")
# define ERR_BADCHANNELKEY(nickname) (":localhost 475 " + nickname + " :Bad channel key")

//WHOIS/WHOWAS Replies.

# define RPL_WHOISUSER(nickname, target, user, host, realname) \
(":localhost 311 " + nickname + " " + target + " " + user + " " + host + " * :" + realname)

# define RPL_WHOISPERATOR(nickname, target) (":localhost 313 " + nickname + " " + target + " :is and IRC operator")
# define RPL_ENDOFWHOIS(nickname) (":localhost 318 " + nickname + " :End of /WHOIS list")