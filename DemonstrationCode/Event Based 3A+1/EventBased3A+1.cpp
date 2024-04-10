#include "EventBased3A+1.h"

//declare variables in global scope
bool serverRunning = true;
ClientNode *headPtr = NULL;
ClientNode *lastPtr = NULL;
EventManager *eventManager = NULL;

//prints string when debug macro is enabled
void debug(string str)
{
    #if DEBUG
    cout << str << endl <<endl;
    #endif
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
    //connMutex.lock();
    ClientNode *wkgPtr = headPtr;
    ClientNode *wkgPtr2;
    
    //loop through the list
    while (wkgPtr != NULL)
    {
        //save location of next node
        wkgPtr2 = wkgPtr->nextPtr;

        if (eventManager != NULL)
        {
            eventManager->stopMonitoring(wkgPtr->conn->socketId);
        }

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
bool freeClientNode( const int searchId)
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

        eventManager->stopMonitoring(wkgPtr->conn->socketId);

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
            
            eventManager->stopMonitoring(wkgPtr->conn->socketId);

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

//allocate new client node to ll
void addClientNode( Socket *conn )
{
    ClientNode *newNode = new ClientNode;

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

void forget(int clientSocket)
{
    if (eventManager != NULL)
    {
        eventManager->stopMonitoring(clientSocket); 
    }         
    close(clientSocket);
    freeClientNode(clientSocket);
    debug("Client socket " + std::to_string(clientSocket) + " disconnected.\n");
}

//runs the 3A + 1 algorithm and returns the number of iterations before the input number = 1
int getOperationResult(int userInput)
{
    int wkgInt = userInput;
    int i=0;

    //check for invalid input
    if (wkgInt < 1)
    {
        return -1;
    }
    
    //loop until the solution is found or we decide there is no solution
    while (wkgInt != 1 && i < MAX_NUM_ITERATIONS)
    {
        //check for even num
        if(wkgInt % 2 == 0)
        {
            wkgInt = wkgInt / 2;
        }
        //otherwise odd num
        else
        {
            wkgInt = wkgInt *3 +1;
        }
        
        //increment iteration
        i++;
    }
    
    if (i>=MAX_NUM_ITERATIONS)
        {
            return -1;
        }

    return i;
}

int main(int argc, char** argv) 
{
    //init vars
    int clientSocket, result;
    string receivedString;
    Socket *tmpConn;
    ClientNode *wkgPtr;
    int numClients=0;

    snitch();
    
    //startup our server
    Server server(PORT, NUM_CONNECTIONS, true, true);

    //start event manager for server 
    eventManager = new EventManager(server.socketId, NUM_CONNECTIONS);

    debug("Now listening for client connections on port: " + to_string(PORT) + "\n");
    
    //run server execution loop
    while (serverRunning)
    {
        //wait for event
        clientSocket = eventManager->waitForEvent();

        //check for new connection requests
        if (clientSocket == CONN_ATTEMPT)
        {
            //accept client and check if fail
            if (!server.acceptConnection(&clientSocket))
            {
                debug("Error accepting client");
            }
            //success accepting client
            else
            {
                debug("New client on socket: " + to_string(clientSocket));

                tmpConn = new Socket(clientSocket, false);
                
                //send welcome msg check for fail
                if (!tmpConn->sendString(WELCOME_MSG))
                {
                    debug("Client connection failed to be established.");
                    
                    //client failed to send username, close conn
                    delete tmpConn;
                }
                //otherwise client officially connected
                else
                {
                    //setup event listener for incoming messages from client
                    eventManager->monitorClient(clientSocket);
                    
                    //add new connection to list
                    addClientNode(tmpConn);

                    numClients++;

                    debug("Number of connections: " + to_string(numClients));
                }
            }
        }
        //otherwise handle event from existing client
        else
        {
            //get node associated with clientSocket
            wkgPtr=getClientNodeById(clientSocket);

            //handle empty case
            if (wkgPtr == NULL)
            {
                debug("Error: event triggered by nonexistent client!");
                forget(clientSocket);
                numClients--;
            }
            //get data from client, check for error
            else if (!wkgPtr->conn->getString(receivedString))
            {                    
                //free everything associated with client
                forget(clientSocket);
                numClients--;
            }
            //otherwise input successfully taken from client
            else
            {               
                //calculate 3A+1 result
                result = getOperationResult(atoi(receivedString.c_str()));

                //return result to client, check for fail
                if(!wkgPtr->conn->sendString(to_string(result)))
                {
                    forget(clientSocket);
                    numClients--;
                }
                else
                {
                    debug("result sent to client " + to_string(clientSocket) + ": " + to_string(result));
                }
            }
        }
    }

    freeClientNodes();

    return 0;
}