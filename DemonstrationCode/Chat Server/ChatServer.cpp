#include "ChatServer.h"

//declare variables in global scope
ClientNode *headPtr = NULL;
ClientNode *lastPtr = NULL;
bool serverRunning = true;
int numCurrentConnections=0;
//init an epoll instance which has a queue NUM_CONNECTIONS long
int epollFD = epoll_create(NUM_CONNECTIONS);

//prints string when debug macro is enabled
void debug(string str)
{
    #if DEBUG
    cout << str << endl <<endl;
    #endif
}

//return true if given username is found currently connected
bool checkForUsername(const string usr)
{   
    ClientNode *wkgPtr = headPtr;

    while(wkgPtr != NULL)
    {
        if (wkgPtr->username == usr)
        {
            return true;
        }
        wkgPtr = wkgPtr->nextPtr;
    }
    return false;
}

//returns the node associated with given id, otherwise returns null
ClientNode *getClientNodeById(const int searchId)
{
    ClientNode *wkgPtr = headPtr;

    while (wkgPtr != NULL)
    {
        if (wkgPtr->conn->socketId == searchId)
        {
            return wkgPtr;
        }
        wkgPtr = wkgPtr->nextPtr;
    }
    return NULL;
}

//frees sockets and other resources in client linked list
ClientNode *freeClientNodes()
{
    ClientNode *wkgPtr = headPtr;
    ClientNode *wkgPtr2;
    
    //loop through the list
    while (wkgPtr != NULL)
    {
        //save location of next node
        wkgPtr2 = wkgPtr->nextPtr;

        //remove the fd from the epoll monitoring
        epoll_ctl(epollFD, EPOLL_CTL_DEL, wkgPtr->conn->socketId, NULL);

        //delete the socket itself
        delete wkgPtr->conn;

        //free mem allocated to the node
        free(wkgPtr);

        //iterate to next node
        wkgPtr = wkgPtr2;
    }
    //return empty list
    return headPtr;
}

//free single node with given id, maintain linked list integrity
bool freeClientNode(const int searchId)
{
    ClientNode *wkgPtr = headPtr;
    
    if (headPtr == NULL)
    {
        return false;
    }
    //we found the socket at the head of the linked list
    else if (headPtr->conn->socketId == searchId)
    {
        headPtr = headPtr->nextPtr;

        //remove the fd from the epoll monitoring
        epoll_ctl(epollFD, EPOLL_CTL_DEL, wkgPtr->conn->socketId, NULL);

        //delete the socket itself
        delete wkgPtr->conn;

        //free mem allocated to the node
        free(wkgPtr);
        return true;
    }

    ClientNode *wkgPtr2 = wkgPtr;
    wkgPtr = wkgPtr->nextPtr;
    
    //loop through the list
    while (wkgPtr != NULL)
    {
        //we found the socket in the middle of linked list or at the end
        if (wkgPtr->conn->socketId == searchId)
        {
            wkgPtr2->nextPtr = wkgPtr->nextPtr;
            
            //remove the fd from the epoll monitoring
            epoll_ctl(epollFD, EPOLL_CTL_DEL, wkgPtr->conn->socketId, NULL);

            //delete the socket itself
            delete wkgPtr->conn;

            //free mem allocated to the node
            free(wkgPtr);

            return true;
        }

        wkgPtr2 = wkgPtr;
        wkgPtr = wkgPtr->nextPtr;
    }

    return false;
}

//sends a message to all connected clients on behalf of the sending client
void forwardMessage(const ClientNode *sender, const string message, const bool systemMessage)
{
    ClientNode *wkgPtr = headPtr;

    if (headPtr == NULL)
    {
        debug("forwardMessage() called with no active connections, returning");
    }

    int i = 0;
    
    //loop through all members
    while (wkgPtr != NULL)
    {
        //debug("entered the forwarding loop");

        if (wkgPtr != sender)
        {
            if (systemMessage)
            {
                //send msg to current user
                wkgPtr->conn->sendString("------- " +message+" -------");
                debug("message forwarded to " + wkgPtr->username + " from system");
            }
            else
            {
                //send message from sender to current user
                wkgPtr->conn->sendString(sender->username + " : " + message);
                debug("message forwarded to " + wkgPtr->username + " from " + sender->username);
            }
        }

        wkgPtr = wkgPtr->nextPtr;
        i++;
    }

    debug(to_string(i) + " active connections.");
}

//allocate new client node to ll
void addClientNode(const string username, Server *conn )
{
    ClientNode *newNode = new ClientNode;

    newNode->username = username;
    newNode->conn = conn;
    newNode->nextPtr = NULL;

    if (headPtr == NULL)
    {
        headPtr = newNode;
        lastPtr = newNode;
    }
    else
    {
        lastPtr->nextPtr = newNode;
        lastPtr = newNode;
    }
}

int main(int argc, char** argv) 
{
    //init vars
    int clientSocket;
    string tmpStr;
    string receivedString;
    Server *tmpConn;
    ClientNode *wkgPtr;

    // Set signal handler for SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    //startup our server
    Server server(PORT, NUM_CONNECTIONS, true, true);

    cout << "Now listening for client connections on port: " << PORT << endl << endl;

    //set our server socket to be the fd we monitor for incoming events
    struct epoll_event listenEvent;
    listenEvent.data.fd = server.socketId; // Set the file descriptor to monitor
    listenEvent.events = EPOLLIN; // Specify the event(s) to monitor for

    // Add the server socket to the epoll instance
    epoll_ctl(epollFD, EPOLL_CTL_ADD, server.socketId, &listenEvent);
    
    // Allocate memory for an array of epoll events to store event notifications
    struct epoll_event* events = (epoll_event*)malloc(MAX_FDS*sizeof(struct epoll_event));

    //run event loop
    while (serverRunning)
    {
        //wait for events, numEvents will store the number of events that occurred
        int numEvents = epoll_wait(epollFD, events, MAX_FDS, -1);

        //loop through all events that occurred
        for (int i=0; i<numEvents; i++)
        {
            //check for new connection requests
            if (events[i].data.fd == server.socketId)
            {
                //accept client and check if fail
                if (!server.acceptConnection(&clientSocket))
                {
                    perror("Error accepting client\n");
                }
                //success accepting client
                else
                {
                    cout << "New client\n";

                    // Create an epoll event structure for the accepted client socket
                    struct epoll_event event;
                    event.data.fd = clientSocket; // Associate the client socket with the event
                    event.events = EPOLLIN; // | EPOLLOUTMonitor both read and write events

                    // Add the client socket to the epoll instance to begin monitoring
                    epoll_ctl(epollFD, EPOLL_CTL_ADD, clientSocket, &event);

                    //create tmp conn to get username
                    tmpConn = new Server(clientSocket, false);
                    
                    //get username form client
                    if (!tmpConn->getString(receivedString))
                    {
                        cout << "Client failed to send username\n";
                        
                        //client failed to send username, close conn
                        delete tmpConn;
                    }
                    //otherwise client sent username, check if it is already connected
                    else if (checkForUsername(receivedString))
                    {
                        cout << receivedString + " is already connected. Connection rejected.\n";
                        
                        //client is already connected, reject second conn
                        tmpConn->sendString("The username " + receivedString + " is already connected. Closing connection..");
                        delete tmpConn;
                    }
                    //otherwise this is unique user
                    else
                    {
                        cout << "Client admitted to chat\n";

                        //accept them to the chat
                        tmpConn->sendString("\n\n============ Welcome to the chat " + receivedString + "! ============");

                        numCurrentConnections++;

                        //enable non-blocking mode on socket
                        //fcntl(clientSocket, F_SETFL, O_NONBLOCK);
                        
                       //add new connection to list
                       addClientNode(receivedString, tmpConn);

                        //notify members of new arrival
                        tmpStr =  lastPtr->username + " joined the chat!";
                        forwardMessage(lastPtr, tmpStr, true);
                    }
                }
            }
            //otherwise handle event from existing client
            else
            {
                //get client socket corresponding to current event
                clientSocket = events[i].data.fd;

                //get node associated with clientSocket
                wkgPtr=getClientNodeById(clientSocket);

                //handle empty case
                if (wkgPtr == NULL)
                {
                    debug("Error: event triggered by nonexistent client!");

                    //remove the client socket from epoll monitoring
                    epoll_ctl(epollFD, EPOLL_CTL_DEL, clientSocket, NULL);
                }
                //get data from client, check for error, check for manual disconnect from client
                else if (!wkgPtr->conn->getString(receivedString) || receivedString == "LEAVE" || receivedString == "SHUTDOWN")
                {                    
                    //remove the client socket from epoll monitoring
                    epoll_ctl(epollFD, EPOLL_CTL_DEL, clientSocket, NULL);
                    
                    //close client socket
                    close(clientSocket);

                    //get name of participant that left
                    tmpStr = wkgPtr->username;
                    
                    //delete client node
                    freeClientNode(clientSocket);

                    //alert other members
                    forwardMessage(NULL, tmpStr + " left the chat.", true);

                    //print disconnect message
                    cout << "Client socket " + std::to_string(clientSocket) + " disconnected.\n\n";
                }
                //otherwise data successfully taken, check for shutdown server command
                else if (receivedString == "SHUTDOWN ALL")
                {
                    forwardMessage(NULL, "SERVER IS BEING CLOSED", true);
                    serverRunning = false;
                    freeClientNodes();
                    break;
                }
                //otherwise this is a simple message to all members
                else
                {
                    forwardMessage(wkgPtr, receivedString, false);
                }
            }
        }
    }

    return 0;
}