#include "DemoServer.h"

void *handle_client(void *dereferencedSocket) 
{
    string response;
    string clientMsg;

    int clientSocket = *(int *)dereferencedSocket;
    
    Server socket(clientSocket, true);

    // Print the thread ID of the current thread and client socket
    pthread_t threadId = pthread_self();
    cout << "Thread ID " << threadId << " handling client socket: " << clientSocket << endl;

    // Send initial instructions
    socket.sendString("\nPQC Test Server\n===============\n\nType \"PQC-ON\" or \"PQC-OFF\" to set cryptography.\n");

    // Loop until user disconnects
    while (true) 
    {
        // Get client msg, check for failure
        if (!socket.getString(clientMsg)) 
        {
            // Print disconnect and end loop
            cout << DISCONNECT_MSG << endl;
            break;
        }
        //clientMsg = "PQC-ON";
        //cout << "\"" << clientMsg << "\"\n";
        
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

        response.clear();
    }

    cout << "Closing client connection" << endl;

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