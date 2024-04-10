#include "Client.h"

int main() 
{
    //declare vars
    string input,response;
    Client *client;
       
    // Create a client which will attempt to connect to server on given port, set initial encryption
    client = new Client(SERVER_ADDR, PORT, true);
    
    // get initial instructions from server, exit if fail
    assert(client->getString(response));

    // enter execution loop
    while (true) 
    {
        cout << "\nInput: ";
        
        // Get input from the user
        getline(cin, input);
        
        // Send input to the server
        assert(client->sendString(input));   

        cout << "Result: ";
        
        // Send input to the server
        assert(client->getString(response)); 
    }

    //free client class
    delete client;

    cout << "\nExiting program\n";

    return EXIT_SUCCESS;
}
