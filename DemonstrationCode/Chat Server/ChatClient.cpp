#include "ChatClient.h"

//declare mutex in global scope
mutex mtx;
//exit contidtion of thread
bool stopThread;
//main loop exit condition
bool stopMain = false;

//trim leading and trailing whitespace from string
std::string trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](int c) { return std::isspace(c); });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](int c) { return std::isspace(c); }).base();
    return (start < end ? std::string(start, end) : std::string());
}


//threaded function for updating terminal with messages from other users
DWORD WINAPI listenForMessages(LPVOID dereferencedClient) 
{
    Client* client = reinterpret_cast<Client*>(dereferencedClient);
    string message;

    //enable non-blocking mode on socket
    u_long mode = 1;
    ioctlsocket(client->socketId, FIONBIO, &mode);
    
    //listening loop
    while (!stopThread) 
    {
        mtx.lock();
        
        //if a string comes through, auto print it, then print next terminal line
        if (client->getString(message))
        {
            cout << "> "; //+ getLatestConsoleLine();
        }

        //check for server shutdown
        if (message.find("SERVER IS BEING CLOSED")!= std::string::npos)
        {
           //terminate
           assert(false);
        }

        mtx.unlock();
    }

    mtx.lock();
    cout << "Threaded client listener is returning\n\n";
    mtx.unlock();

    return 0;
}

//retrieves data from properties file (server ip and port)
bool getProperties( string *ip, int *port, string *username)
{
    ifstream propertiesFile;
    string line;

    //open properties file
    propertiesFile.open(PROPERTIES_FILE_NAME);

    // Check if the file opened successfully
    if (!propertiesFile.is_open()) 
    {
        return false; // Exit the program with an error
    }
    
    //loop for 3 lines
    for(int i=0;i<3;i++)
    {
        //get line
        getline(propertiesFile, line);
        
        // Find the position of the colon character
        size_t colonPos = line.find(':');

        //check for a colon on this line
        if (colonPos != string::npos) 
        {
            // Extract the substring starting from the position right after the colon
            string extractedData = trim(line.substr(colonPos + 1));

            // Trim leading whitespace if necessary
            size_t firstNonSpace = extractedData.find_first_not_of(" \t");
            if (firstNonSpace != string::npos) 
            {
                //store extracted property
                extractedData = extractedData.substr(firstNonSpace);
            }

            //if first iteration store in ipaddr
            if(i==0)
            {
                *ip = extractedData;
            }
            //second iteration is port number
            else if (i==1)
            {
                //cout << "\"" << extractedData << "\""<< endl;
                
                *port = stoi(extractedData);
            }
            //third is username
            else 
            {
                *username = extractedData;
            }
        }
        else
        {
            propertiesFile.close();
            return false;
        }
    }
    propertiesFile.close();
    return true;
}

//wait for client to join server
void startWaitingRoom()
{
    string input;

    cout << "You have entered the waiting room, type \"JOIN\" to join the chat\n\n";

    cout << "The chat server info will be taken from " << PROPERTIES_FILE_NAME << ". The file must be in the format...";
    cout << endl << "Server IP: [insert server ip]" << endl;
    cout <<"Server Port: [insert server port]" << endl;
    cout <<"Username: [insert username]" << endl << endl;
    cout << "Setup " << PROPERTIES_FILE_NAME << " now if needed...\n";

    while(input != "JOIN")
    {
        cout << "\nInput: ";
        getline(cin, input);
    }
}

int main() 
{
    //declare vars
    string input,response;
    string serverAddr; 
    string username;
    Client *client;
    int port;
    HANDLE hThread;

    //print title and instructions
    cout << "\nChat Client" << endl << "===========" << endl << endl;
    
    //set label for waiting room
    WAITING_ROOM:

    //wait for JOIN command
    startWaitingRoom();

    //clear screen
    system("cls");

    //get properties and check for fail
    if (!getProperties(&serverAddr, &port, &username))
    {
        cerr << "Error: Unable to extract data from " << PROPERTIES_FILE_NAME << endl;
        return 1; // Exit the program with an error
    }
    
    // Create a client which will attempt to connect to server on given port, set initial encryption
    client = new Client(serverAddr, port, true);
    
    //send username to client to join chat
    assert(client->sendString(username));
    
    // get initial instructions from server, exit if fail
    assert(client->getString(response));

    //check for error
    if (response.find("Closing connection")!= std::string::npos)
        {
           //terminate
           delete client;
           return 1;
        }

    stopThread = false;

    //setup the listener thread (will listen for incoming messages from server and print to terminal)
    hThread = CreateThread(NULL, 0, listenForMessages, client, 0, NULL);

    //check for failed thread
    if (hThread == NULL) {
        std::cerr << "Error: Failed to create thread" << std::endl;
        return 1;
    }

    cout << "> ";

    // enter execution loop
    while (!stopMain) 
    {
        // Get input from the user
        getline(cin, input);

        //lock terminal resource
        mtx.lock();
        cout << "> ";
        
        // Send input to the server
        assert(client->sendString(input));   
        
        //unlock terminal
        mtx.unlock();

        //check for commands
        if (input == "LEAVE" )
        {
            //stop thread
            stopThread = true;
            WaitForSingleObject(hThread, INFINITE); 
            CloseHandle(hThread); 

            //free client class
            delete client;

            //clear screen
            system("cls");

            //reset
            goto WAITING_ROOM;
        }
        else if (input == "SHUTDOWN")
        {
            break;
        }
    }

    //stop thread
    stopThread = true;
    WaitForSingleObject(hThread, INFINITE); 
    CloseHandle(hThread); 

    //free client class
    delete client;

    cout << "\nExiting program\n";

    return EXIT_SUCCESS;
}
