#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

/************************************************************************
 * Libraries
 ************************************************************************/
#include "Socket.h"
#include <thread>
#include <sys/epoll.h>
#include <fcntl.h>
#include <pthread.h>
#include <string>

using namespace std;

/************************************************************************
 * Compilation settings
 ************************************************************************/
#define DEBUG true
/************************************************************************
 * Constants
 ************************************************************************/
#define PORT 777          // port the server will listen on
#define MAX_FDS 200
#define STD_STR_SIZE 100;

// number of pending connections in the connection queue
#define NUM_CONNECTIONS 10

/************************************************************************
 * Enumerations
 ************************************************************************/

/************************************************************************
 * Structures
 ************************************************************************/

struct ClientNode {
    Server *conn;
    string username;
    ClientNode *nextPtr;
};

/************************************************************************
 * Function prototype declarations
 ************************************************************************/ 
ClientNode *freeClientNodes();
bool checkForUsername(const string usr);
ClientNode *getClientNodeById(const int searchId);
bool freeClientNode(const int searchId);
void forwardMessage(const ClientNode *sender, const string message, const bool systemMessage);
void addClientNode(const string username, Server *conn );

#endif /* CHAT_SERVER_H */
