#ifndef DEMO_SERVER_H
#define DEMO_SERVER_H

#include "Socket.h"

using namespace std;

//port server will bind to/listen on
#define PORT 777
#define DISCONNECT_MSG "Client disconnected.\n"

//threaded function for handling a clients connection
void *handle_client(void *dereferencedSocket); 

// number of pending connections in the connection queue
#define NUM_CONNECTIONS 10

#endif /* DEMO_SERVER_H */
