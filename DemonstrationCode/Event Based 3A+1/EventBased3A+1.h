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
#define STD_STR_SIZE 100;

// number of pending connections in the connection queue
#define NUM_CONNECTIONS 10
#define MAX_NUM_ITERATIONS 150

#define WELCOME_MSG "3A + 1 Server\n=============\n\nSend integers to get the 3A + 1 solution.\n"

/************************************************************************
 * Enumerations
 ************************************************************************/

/************************************************************************
 * Structures
 ************************************************************************/

struct ClientNode {
    Socket *conn;
    string username;
    ClientNode *nextPtr;
};

/************************************************************************
 * Function prototype declarations
 ************************************************************************/ 
ClientNode *freeClientNodes();
ClientNode *getClientNodeById(const int searchId);
bool freeClientNode(const int searchId);
void addClientNode(Socket *conn );
int getOperationResult(int userInput);
void forget(int clientSocket);

#endif /* CHAT_SERVER_H */
