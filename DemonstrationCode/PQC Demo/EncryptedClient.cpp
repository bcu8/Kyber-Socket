#include "EncryptedClient.h"

int main() 
{
    string input;
    string response;

    // Create a client which will attempt to connect to server on given port
    //cryptography handshake will automatically occur once connection is established
    Client client(SERVER_ADDR, PORT, true);

    // get initial instructions from server, exit if fail
    assert(client.getString(response));

    // enter execution loop
    while (true) 
    {
        // get string input from user
        cout << "\nInput: ";
        getline(cin, input);
               
        // send user input to server, exit if fail
        assert(client.sendString(input));

        //check for keywords
        if (input == "PQC-OFF")
        {
            client.setCryptography(false);
            cout << "PQC disabled on client.\n";
        }
        else if (input == "PQC-ON")
        {
            client.setCryptography(true);
            cout << "PQC enabled on client.\n";
        }
        
        //clear last user input
        input.clear();
        cout << endl;
        
        // get result from server, exit if fail
        assert(client.getString(response));
    }

    return EXIT_SUCCESS;
}
