#pragma once

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <csignal>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#include <exception>

/**
 * LIBRARIES     | FUNCTIONS
 *
 * types, socket | socket, setsockopt, getsockname, bind,
 *                 connect, listen, accept, send, recv
 * csiignal      | signal
 * unistd        | close, lseek
 * fcntl         | fcntl
 * netdb         | getprotobyname, gethostbyname, getaddrinfo, freeaddrinfo
 * arpa/inet     | htons, htonl, ntohs, ntohl
 * netinet/in    | inet_addr, inet_ntoa
 * sys/stat      | fstat
 * poll          | poll
 *
 */
