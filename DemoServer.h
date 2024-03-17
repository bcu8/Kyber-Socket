#ifndef DEMO_SERVER_H
#define DEMO_SERVER_H

/************************************************************************
 * Libraries
 ************************************************************************/
#include "Socket.h"

using namespace std;

/************************************************************************
 * Function prototype declarations
 ************************************************************************/
void *handle_client(void *dereferencedSocket); 

/************************************************************************
 * Constants
 ************************************************************************/
#define PORT 777             // port the server will listen on

#define DISCONNECT_MSG "Client disconnected.\n"

// number of pending connections in the connection queue
#define NUM_CONNECTIONS 10

/************************************************************************
 * Enumerations
 ************************************************************************/

#endif /* DEMO_SERVER_H */
