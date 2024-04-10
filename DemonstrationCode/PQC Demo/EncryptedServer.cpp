#include "EncryptedServer.h"

//threaded function for handling a clients connection
void *handle_client(void *dereferencedSocket) 
{
    string response;
    string clientMsg;

    //get socket id from dereferenced parameter
    int clientSocket = *(int *)dereferencedSocket;
    
    //declare a socket class
    Server socket(clientSocket, true);

    // Print the thread ID of the current thread and client socket
    pthread_t threadId = pthread_self();
    cout << "Thread ID " << threadId << " handling client socket: " << clientSocket << endl << endl;

    // Send initial instructions to client
    socket.sendString("\nPQC Test Server\n===============\n\nType \"PQC-ON\" or \"PQC-OFF\" to set cryptography.\n");

    // Loop until user disconnects
    while (true) 
    {
        // Get client msg, check for failure
        if (!socket.getString(clientMsg)) 
        {
            //end loop
            break;
        }
        
        //check for keywords
        if (clientMsg == "PQC-OFF")
        {
            socket.setCryptography(false);
            response = "PQC disabled on server.";
        }
        else if (clientMsg == "PQC-ON")
        {
            socket.setCryptography(true);
            response = "PQC enabled on server.";
        }
        //otherwise continue conversation
        else
        {
            cout << "Enter Response: ";
            getline(cin, response);
        }

        // Send response to client
        socket.sendString(response);

        //clear user input
        response.clear();
    }

    //print disconnect msg
    cout << DISCONNECT_MSG << endl << endl;

    // Exit thread
    pthread_exit(NULL);
}

int main(int argc, char** argv) 
{
    int clientSocket;
    
    // Set signal handler for SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    Server server(PORT, NUM_CONNECTIONS, true, true);

    cout << "Now listening for client connections on port: " << PORT << endl << endl;

    // Server loop
    while (true) 
    {
        // Accept connection from client, check if unsuccessful
        if ( !server.acceptConnection(&clientSocket) ) 
        {
            perror("Error accepting client\n");
        } 
        //otherwise successful connection established
        else 
        {
            cout << "\nAccepted client" << endl;

            // Initialize thread var
            pthread_t conThread;

            // Create new thread to handle client's session, check for unsuccessful operation
            if (pthread_create(&conThread, NULL, handle_client, (void *)&clientSocket) != 0) 
            {
                perror("Error creating thread for client connection. Closing socket\n");
                close(clientSocket);
            } 
            //otherwise we initialized thread successfully
            else 
            {
                // Thread created successfully, handle_client has started
                // Detach allows thread to automatically release its resources when handle_client function returns
                pthread_detach(conThread);
            }
        }
    }

    return 0;
}