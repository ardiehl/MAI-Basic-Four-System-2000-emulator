/***************************************************************************
 *  socket_connections.h
 *
 *  Created: Aug, 28 2026 AD
 *  Changed:
 *  Armin Diehl <ad@ardiehl.de>
 ****************************************************************************
 * TCP telnet connections for the eagle emulator
 */


#ifndef SOCKET_CONNECTIONS_H_INCLUDED
#define SOCKET_CONNECTIONS_H_INCLUDED

//#define SOCK_DEBUG

// for POLLRDHUP
//#define _GNU_SOURCE
#include <stdbool.h>
#include "sim.h"
// we have 2 ports on CMB and 2 fourway controllers
#define SOCK_MAX 10
#define SOCK_DEFAULT_BASE_PORT 4000

// init: setup any port to listen for a new connection, startingPortNumber can be 0 for the default
void sock_init(int startingPortNumber);

// deinit: close all open sockets
void sock_deinit();

// check for incoming connections or data on all open ports and set the status field for each connection
void sock_poll();

// read a byte, returns 1 if data was available
int sock_getchar(int portNum, char * data);

void sock_putchar(int portNum, char data);

void sock_putstr(int portNum, char * data);

int sock_dbgCmd(int numArgs, struct args_t * args);

#endif // SOCKET_CONNECTIONS_H_INCLUDED
