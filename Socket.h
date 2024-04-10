/*********************************************************************************
 * File: Socket.h
 * Author: Brandon Udall 
 * Created: 4/7/2024
 * Last Modified: 3/16/2024
 * Version: 1.0
 * 
 * Dependencies: 
 * OpenSSL library
 * kyber Github library 
 *     Repo: https://github.com/itzmeanjan/kyber
 *     License: CCO 1.0 Universal
 *     Author: itzmeanjan
 * 
 * Usage: (you may need to upgrade your compiler to support C++20)
 * This class provides socket functionality for network communication.
 * The code is designed to compile automatically on Linux or Windows 
 * through the use of compiler directives.
 * Adjust the compilation options below to fit your needs then include
 * this header along with its dependencies within your code. Read the 
 * comments to get a clear picture of how to use the Client and Server
 * sub-classes.
 * 
 * License Information:
 * This code is provided under the MIT License.
 * You are free to use, modify, and distribute this code for any purpose, 
 * including commercial purposes, with or without attribution, as long as 
 * the original copyright notice and license information are included.
 * 
 * Disclaimer:
 * This software is provided "as is," without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose, and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages, or other
 * liability, whether in an action of contract, tort, or otherwise, arising
 * from, out of, or in connection with the software or the use or other dealings
 * in the software.
********************************************************************************/

#ifndef SOCKET_H
#define SOCKET_H

/************************************************************************
 * Compilation options
 ************************************************************************/
//toggle compiling cryptography support functions and variables
#define CRYPTOGRAPHY false

//true = debugging information will be printed to terminal
#define VERBOSE false

//toggle compilation of event based support classes (only available on linux)
#define EVENT_BASED true
//select event based architecture (0 for epoll, 1 for poll, 2 for select)
#define EVENT_BASED_ARCHITECTURE 2

#if EVENT_BASED_ARCHITECTURE == 0
    #define archType "EPOLL IS IN USE"
#elif EVENT_BASED_ARCHITECTURE ==1
    #define archType "POLL IS IN USE"
#else
    #define archType "SELECT IS IN USE"
#endif
#if EVENT_BASED
#include <iostream>
void snitch()
{
    std::cout << std::endl<< archType << std::endl;
}
#endif
/************************************************************************
 * Headers
 ************************************************************************/

#include <iostream>
#include <string>

// read/write/close
#include <sys/types.h>
#include <cstring>

#if CRYPTOGRAPHY
#include "kyber/kyber1024_kem.hpp"
#include <algorithm>
#include <openssl/aes.h>
#include <openssl/evp.h>   // For EVP functions (EVP_CIPHER_CTX_new, EVP_EncryptInit_ex, EVP_DecryptInit_ex, etc.)
#include <openssl/rand.h>  // For RAND_bytes function used for generating random bytes
#include <openssl/err.h>
#endif

#if EVENT_BASED && defined(__linux__)
#if EVENT_BASED_ARCHITECTURE == 0
#include <sys/epoll.h>
#include <fcntl.h>
#elif EVENT_BASED_ARCHITECTURE == 1
#include <poll.h>
#include <vector>
#else
#include <vector>
#include <sys/select.h>
#endif
#endif

#ifdef _WIN32
    #include <Winsock2.h>
    #include <ws2tcpip.h>
#elif defined(__linux__)
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <signal.h>
    #include <netdb.h>//getaddrinfo()
    #include <sys/uio.h>
    #include <unistd.h>
#endif

using namespace std;

/************************************************************************
 * Constants and Enums
 ************************************************************************/

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#if EVENT_BASED 
const int CONN_ATTEMPT = -100;
const int MAX_FDS = 100;
#endif

#if CRYPTOGRAPHY
constexpr size_t SEED_LEN = 32;
constexpr size_t KEY_LEN = 32;
#endif

/************************************************************************
 * Socket class declaration
 ************************************************************************/

//socket is the parent of server and client subclasses. You cant declare a Socket class directly.
//you must use one of the subclasses. Use alternate Server() constructor in the case of a server 
//accepting client connections and passing them to threads... other than that its self explanitory (call server for server, client for client)
class Socket {
public:
    //Use this socket constructor when you want to interact with an established socket using class functionality
    Socket( const int socket, const bool autoPrint)
    {
        socketId = socket;
        autoPrintResponses = autoPrint;

        #if defined(__linux__)
        //prevent termination if writing to closed socket
        signal(SIGPIPE, SIG_IGN);
        #endif

        #if CRYPTOGRAPHY
        initiator = true;
        applyCryptography = true;
        
        //collaborate with connected party to get shared encryption key
        setupEncryption();
        #endif
    }

    //this destructor is inherited for server and client subclasses
    ~Socket() 
    {
        #if CRYPTOGRAPHY
        freeEncryptionContext();
        #endif

        //free socket
        #ifdef _WIN32
        closesocket(socketId);
        WSACleanup();
        #else
        close(socketId);
        #endif
    }

protected:
    //Socket sub-classes will inherit this constructor, then run their own specialized constructors
    Socket()
    {
        //assign temp val to prevent compile warnings
        socketId = -101;

        #if defined(__linux__)
        //prevent termination if writing to closed socket
        signal(SIGPIPE, SIG_IGN);
        #endif
    }

    #if CRYPTOGRAPHY
    // Define the encryption context structure
    struct CryptographyContext {
        EVP_CIPHER_CTX *encrypt_ctx;
        EVP_CIPHER_CTX *decrypt_ctx;
        std::vector<uint8_t> sharedKey;
        unsigned char iv[AES_BLOCK_SIZE];
    };

    bool initiator;
    bool applyCryptography;

    // Define the encryption context structure
    CryptographyContext encryptionContext;
    bool initAES();
    bool sendKeyData(const uint8_t* data, size_t dataSize);
    bool getKeyData(uint8_t* data, size_t dataSize);
    bool encrypt(string& str);
    bool decrypt(string& str);
    void setupEncryption();
    void freeEncryptionContext();
    #if VERBOSE
    void printHex(string str);
    #endif //verbose
    #endif //cryptography

public:
    int socketId;
    bool autoPrintResponses;

    bool getString(string& str);
    bool sendString(string str);
    #if CRYPTOGRAPHY
    void setCryptography(bool cryptography);
    #endif
};

/************************************************************************
 * Client sub-class declaration
 ************************************************************************/

class Client : public Socket {
public:
    //constructor, attempts to establish connection to given server on given port
    //if fail, runtime exception is thrown. Uses compiler directives to implement
    //windows or linux sockets based on the platform the caller is using
    Client(const string serverIp, const int port, const bool autoPrint)
    : Socket()
    {
        autoPrintResponses = autoPrint;
        
        //check for windows os, if so run windows socket creation and establish connection
        #ifdef _WIN32 
            struct sockaddr_in server_address;
            WSADATA wsaData;
            int iResult;

            // Initialize Winsock
            iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (iResult != 0) 
            {
                cerr << "WSAStartup failed: " << iResult << endl;
                throw runtime_error("WSAStartup failed");
            }

            // Create a socket for the client
            socketId = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (socketId == (int)INVALID_SOCKET) 
            {
                cerr << "Error at socket(): " << WSAGetLastError() << endl;
                WSACleanup();
                throw runtime_error("Failed to create socket");
            }

            // Set up the server address structure
            memset(&server_address, 0, sizeof(server_address));
            server_address.sin_family = AF_INET;
            server_address.sin_addr.s_addr = inet_addr(serverIp.c_str());
            server_address.sin_port = htons(port);

            // Connect to server
            iResult = connect(socketId, reinterpret_cast<struct sockaddr*>(&server_address), sizeof(server_address));
            if (iResult == SOCKET_ERROR) 
            {
                cerr << "Unable to connect to server: " << WSAGetLastError() << endl;
                closesocket(socketId);
                WSACleanup();
                throw runtime_error("Failed to connect to server");
            }

        //check for linux, if so run linux socket creation and establish connection
        #elif defined(__linux__) 
            struct sockaddr_in client_address;
            
            // create an unnamed socket, and then name it
            socketId = socket(AF_INET, SOCK_STREAM, 0);

            // check for socket creation failure
            if (socketId == -1) 
            {
                cerr << "Error at socket(): " << strerror(errno) << endl;
                throw runtime_error("Failed to create socket");
            }

            // create addr struct
            memset(&client_address, 0, sizeof(client_address));
            client_address.sin_family = AF_INET;
            client_address.sin_addr.s_addr = inet_addr(serverIp.c_str());
            client_address.sin_port = htons(port);

            // connect to server socket
            if (connect(socketId, (struct sockaddr *)&client_address, sizeof(client_address)) == -1) 
            {
                cerr << "Unable to connect to server: " << strerror(errno) << endl;
                close(socketId);
                throw runtime_error("Failed to connect to server");
            }
        #endif

        #if CRYPTOGRAPHY
        initiator = false;

        applyCryptography = true;

        //collaborate with connected party to get shared encryption key
        setupEncryption();
        #endif
    }

    // Destructor for the Client class
    ~Client() {
        // Call the Socket destructor explicitly
        Socket::~Socket();
    }
};

/************************************************************************
 * Server sub-class declaration
 ************************************************************************/

class Server : public Socket {
public:
    // Constructor, creates socket, binds to port, then listens for incoming connections. 
    Server(const int port, const int numServerThreads, bool autoPrint, bool portReuse)
    : Socket()
    {   
        autoPrintResponses = autoPrint;
        
        #if CRYPTOGRAPHY
        applyCryptography = true;

        initiator = true;
        #endif
        
        struct sockaddr_in server_address;

        #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
        #endif

        // Create an unnamed socket
        socketId = socket(AF_INET, SOCK_STREAM, 0);

        if (socketId == -1) {
            #ifdef _WIN32
            WSACleanup();
            #endif
            throw std::runtime_error("Error creating socket");
        }

        // Configure socket
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = htonl(INADDR_ANY); // Accept clients on any interface
        server_address.sin_port = htons(port); // Port to listen on

        if (portReuse) {allowPortReuse();}

        // Bind to port
        if (bind(socketId, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
            #ifdef _WIN32
            closesocket(socketId);
            WSACleanup();
            #else
            close(socketId);
            #endif
            throw std::runtime_error("Error binding socket");
        }

        // Listen for client connections (pending connections get put into a queue)
        if (listen(socketId, numServerThreads) == -1) {
            #ifdef _WIN32
            closesocket(socketId);
            WSACleanup();
            #else
            close(socketId);
            #endif
            throw std::runtime_error("Error listening on socket");
        }
    }

    // Destructor for the Server class
    ~Server() {
        // Call the Socket destructor explicitly
        Socket::~Socket();
    }

    void allowPortReuse();
    bool acceptConnection(int *client_socket);
};

/************************************************************************
 * Socket Methods (available to both Client and Server subclasses)
 ************************************************************************/

//waits for string from connected socket. Incoming string must be followed by '\0' (as done in sendString())
inline bool Socket::getString(string& str) {
    // Clear string before using it
    str.clear();
    bool result = true;
    char currentChar;
    
    //receive string character by character until null char is received
    while(recv(socketId, &currentChar, sizeof(currentChar), 0) > 0)
    {
        //break if null char
        if (currentChar == '\0')
        {
            break;
        }
        //append char to string
        str += currentChar;
    }

    if (str.length() == 0)
    {
        return false;
    }
    
    #if VERBOSE
        cout << "String received: " << str << endl << endl;
    #endif
    
    #if CRYPTOGRAPHY
    // Decrypt the message
    result = decrypt(str);
    #endif

    // Print if autoPrintResponses is true
    if (autoPrintResponses) {
        cout << str << endl;
    }

    return result;
}

//send given string to connected socket (terminate transmission with \0)
inline bool Socket::sendString(string str) 
{
    bool result = true;

    #if CRYPTOGRAPHY
    // Encrypt the string
    result = encrypt(str);
    #endif

    #if VERBOSE
        cout << "String to send: " << str << " with length :" << str.length() << endl << endl;
    #endif

    int transmissionLen = str.length() +1;
    
    // Send the data
    int sendResult = send(socketId, str.c_str(), transmissionLen, 0);
    if (sendResult != transmissionLen) {
        return false; // Error in sending data
    }

    return result;
}

/************************************************************************
 * Server Class Methods
 ************************************************************************/
//prevent wait time after restarting server on the same port
void Server::allowPortReuse() 
{
    int reuse = 1;
    #ifdef _WIN32
    if (setsockopt(socketId, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) == SOCKET_ERROR) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }
    #else
    if (setsockopt(socketId, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }
    #endif
}

//wait for client to connect, make a client_socket when they do. If fail to connect, return false
bool Server::acceptConnection(int *client_socket)
{
    //check for failed client connection
    if ((*client_socket = accept(socketId, NULL, NULL)) == -1) 
        {
            return false;
        } 
    return true;
}

/************************************************************************
 * Event Manager Classes (Can be declared and used alongside server to create
 * an event based server)
 ************************************************************************/

#if EVENT_BASED && defined(__linux__)

/************************************************************************
 * Epoll implementation
 ************************************************************************/

#if EVENT_BASED_ARCHITECTURE == 0
class EventManager {

public:
    //declare vars
    struct epoll_event newConnectionEvent;
    struct epoll_event *events;
    int epollFD;
    int serverSocket;
    int pendingEvents;

    //wait for event and return the type of event that occurred   
    int waitForEvent();

    //creates a new epoll instance for monitoring client socket for events
    void monitorClient(int clientSocket);
    void stopMonitoring(int clientSocket);

    //default constructor initializes the epoll instance and events struct
    //the server socket will be monitored for incoming events, i.e. connection requests
    //and messages from client
    EventManager(int socket, int maxConnections)
    {
        serverSocket = socket;

        //init an epoll instance which has a queue NUM_CONNECTIONS long
        epollFD = epoll_create(maxConnections);
        
        // Set the file descriptor to monitor
        newConnectionEvent.data.fd = serverSocket;

        // Specify the event(s) to monitor for
        newConnectionEvent.events = EPOLLIN; 

        // Add the server socket to the epoll instance
        epoll_ctl(epollFD, EPOLL_CTL_ADD, serverSocket, &newConnectionEvent);
        
        // Allocate memory for an array of epoll events to store event notifications
        events = (epoll_event*)malloc(MAX_FDS*sizeof(struct epoll_event));
    }

    //destructor
    ~EventManager()
    {
        // Close the epoll instance
        close(epollFD);

        // Free the memory allocated for the events array
        free(events);
    }
};

int EventManager::waitForEvent()
{
    pendingEvents = epoll_wait(epollFD, events, MAX_FDS, -1);

    //check for new connection request
    if (events[0].data.fd == serverSocket) 
    {
        return CONN_ATTEMPT;
    }
    //otherwise we received data from an existing client connection
    else 
    {
        //return client socket
        return events[0].data.fd;
    }
}

void EventManager::monitorClient(int clientSocket)
{
    // Create an epoll event structure for the client socket
    struct epoll_event event;
    event.data.fd = clientSocket; // Associate the client socket with the event struct
    event.events = EPOLLIN; //Monitor for incoming data from client

    // Add the client socket to the epoll instance to begin monitoring
    epoll_ctl(epollFD, EPOLL_CTL_ADD, clientSocket, &event);
}

void EventManager::stopMonitoring(int clientSocket)
{
    //remove the client socket from epoll monitoring
    epoll_ctl(epollFD, EPOLL_CTL_DEL, clientSocket, NULL);
}

/************************************************************************
 * poll implementation (epoll is more efficient, consider using that)
 ************************************************************************/

#elif EVENT_BASED_ARCHITECTURE == 1 

#define MAX_FDS 1024 // Maximum number of file descriptors to monitor

class EventManager {
public:
    //declare vars
    int serverSocket;
    std::vector<pollfd> pollFds; // Vector to store pollfd structures for each socket

    //wait for event and return the type of event that occurred   
    int waitForEvent();

    //creates a new pollfd structure for monitoring client socket for events
    void monitorClient(int clientSocket);
    void stopMonitoring(int clientSocket);

    //default constructor initializes the pollfds vector
    //the server socket will be monitored for incoming events, i.e. connection requests
    //and messages from client
    EventManager(int socket, int maxConnections)
    {
        serverSocket = socket;

        // Add the server socket to the pollfds vector
        pollfd serverPollFd;
        serverPollFd.fd = serverSocket;
        serverPollFd.events = POLLIN; // Monitor for incoming data
        pollFds.push_back(serverPollFd);
    }

    //destructor
    ~EventManager()
    {
        // No need to close anything for poll-based implementation
    }
};

int EventManager::waitForEvent()
{
    // Call poll to wait for events
    int readyFds = poll(pollFds.data(), pollFds.size(), -1);
    if (readyFds > 0) {
        // Check which file descriptor has an event
        for (size_t i = 0; i < pollFds.size(); ++i) {
            if (pollFds[i].revents & POLLIN) {
                // Check if it's the server socket
                if (pollFds[i].fd == serverSocket) {
                    return CONN_ATTEMPT;
                } else {
                    // Otherwise, return client socket
                    return pollFds[i].fd;
                }
            }
        }
    }
    return -1; // No event occurred or error
}

void EventManager::monitorClient(int clientSocket)
{
    // Create a new pollfd structure for the client socket
    pollfd clientPollFd;
    clientPollFd.fd = clientSocket;
    clientPollFd.events = POLLIN; // Monitor for incoming data
    pollFds.push_back(clientPollFd);
}

void EventManager::stopMonitoring(int clientSocket)
{
    // Find the pollfd associated with the client socket and remove it
    for (auto it = pollFds.begin(); it != pollFds.end(); ++it) {
        if (it->fd == clientSocket) {
            pollFds.erase(it);
            break;
        }
    }
}

/************************************************************************
 * select implementation (archaic and outdated. Only use on legacy systems)
 ************************************************************************/

#elif EVENT_BASED_ARCHITECTURE == 2 //select

class EventManager {
public:
    int serverSocket;
    int max_fd;
    fd_set readfds;
    std::vector<int> clientSockets;

    EventManager(int socket, int maxConnections) : serverSocket(socket), max_fd(socket)
    {
        FD_ZERO(&readfds);
    }

    // Wait for events on server and client sockets
    int waitForEvent() {
    while (true) {
        FD_SET(serverSocket, &readfds);

        // Add all client sockets to the set
        for (int clientSocket : clientSockets) {
            FD_SET(clientSocket, &readfds);
            if (clientSocket > max_fd) {
                max_fd = clientSocket;
            }
        }

        timeval timeout;
        timeout.tv_sec = 3; // 3 seconds timeout
        timeout.tv_usec = 0;

        int result = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
        if (result == -1) {
            std::cerr << "Error in select" << std::endl;
            return -1;
        } else if (result > 0) {
            // Check if server socket has an event
            if (FD_ISSET(serverSocket, &readfds)) {
                return CONN_ATTEMPT;
            }

            // Check if any client socket has an event
            for (int clientSocket : clientSockets) {
                if (FD_ISSET(clientSocket, &readfds)) {
                    return clientSocket; // Return the client socket with the event
                }
            }
        }
        // If no event occurred, continue looping
    }
}

    void monitorClient(int clientSocket) {
    // Add the client socket to the list of monitored sockets
    clientSockets.push_back(clientSocket);

    // Add the new client socket to the set of file descriptors to monitor
    FD_SET(clientSocket, &readfds);

    // Update max_fd if necessary
    if (clientSocket > max_fd) {
        max_fd = clientSocket;
    }
}

    void stopMonitoring(int clientSocket) {
        // No need to stop monitoring individual clients with select
    }
};

#endif //event based architecture decision

#endif //event based 


/************************************************************************
 * Socket Cryptography Methods (available to both server and client sub-classes)
 ************************************************************************/

#if CRYPTOGRAPHY
// Function to send the public key over the network
bool Socket::sendKeyData(const uint8_t* data, size_t dataSize) {
    // Send the actual data
    int bytesSent = send(socketId, reinterpret_cast<const char*>(data), dataSize, 0);
    if (bytesSent == -1) {
        std::cerr << "Error sending data" << std::endl;
        return false;
    }

    #if VERBOSE
    cout << "public key sent." << endl;
    #endif

    return true;
}

// Function to receive the public key from the network
bool Socket::getKeyData(uint8_t* data, size_t dataSize) 
{
    // Receive key data
    int bytesReceived = recv(socketId, reinterpret_cast<char*>(data), dataSize, 0);
    
    //check for success
    if (bytesReceived <= 0) 
    {
        if (bytesReceived == 0) 
        {
            std::cerr << "Connection closed by peer" << std::endl;
        } else 
        {
            std::cerr << "Error receiving data" << std::endl;
        }
        return false;
    }

    #if VERBOSE
    cout << "public key received: " << data;
    #endif

    return true;
}

#if VERBOSE
void Socket::printHex(string str)
{
    for (char c : str) {
        cout << hex << setw(2) << setfill('0') << static_cast<unsigned int>(static_cast<unsigned char>(c));
    }
    cout << endl;
}
#endif

// Encrypts message
inline bool Socket::encrypt(string& str) {
    if (!applyCryptography) {
        return true; // If cryptography is not applied, consider it successful
    }

    unsigned char cipherText[500]; 
    const unsigned char* plainText = reinterpret_cast<const unsigned char*>(str.c_str());
    int plainTextLength = str.length();
    int cipherTextLength;

    #if VERBOSE
    cout << "plain text input to encrypt(): " << str << endl;
    #endif

    if (EVP_EncryptUpdate(encryptionContext.encrypt_ctx, cipherText, &cipherTextLength, plainText, plainTextLength) != 1)
    {
        cout << "Error: Failed EncryptUpdate";
        return false;
    }
    
    int updateLen;
    if (EVP_EncryptFinal_ex(encryptionContext.encrypt_ctx, cipherText + cipherTextLength, &updateLen) != 1)
    {
        cout << "Error: Failed EncryptFinal";
        return false;
    }
    cipherTextLength += updateLen;

    // Update the input string with the encrypted data
    str.assign(reinterpret_cast<char*>(cipherText), cipherTextLength);

    #if VERBOSE
    cout << "Encrypted text output from encrypt(): ";
    printHex(str);
    #endif

    return true;
}

// Decrypts message
inline bool Socket::decrypt(string& str) {
    if (!applyCryptography) {
        return true; // If cryptography is not applied, consider it successful
    }

    #if VERBOSE
    cout << "Encrypted text input to decrypt(): ";
    printHex(str);
    #endif

    unsigned char plainText[500]; 
    const unsigned char* cipherText = reinterpret_cast<const unsigned char*>(str.c_str());
    int cipherTextLength = str.length();
    int plainTextLength;

    if (EVP_DecryptUpdate(encryptionContext.decrypt_ctx, plainText, &plainTextLength, cipherText, cipherTextLength) != 1)
    {
        //handle error
        cout << "Error: DecryptUpdate";
        return false;
    }

    int len;
    if(1 != EVP_DecryptFinal_ex(encryptionContext.decrypt_ctx, plainText + plainTextLength, &len))
    {
        //handle error
        cout << "Error: DecryptFinal";
        return false;
    }
    plainTextLength += len;  

    // Set the decrypted data back to the input string
    str.assign(reinterpret_cast<char*>(plainText), plainTextLength);

    #if VERBOSE
    cout << "plain text output from decrypt(): " << str << endl;
    #endif

    return true;
}

//initializes the encryption and decryption context
bool Socket::initAES() {
    // Initialize AES encryption context
    encryptionContext.encrypt_ctx = EVP_CIPHER_CTX_new();
    if (!encryptionContext.encrypt_ctx) {
        std::cerr << "Error: Failed to create encryption context." << std::endl;
        return false;
    }
    if (EVP_EncryptInit_ex(encryptionContext.encrypt_ctx, EVP_aes_256_cbc(), NULL, encryptionContext.sharedKey.data(), encryptionContext.iv) != 1) {
        std::cerr << "Error: Failed to initialize encryption context." << std::endl;
        EVP_CIPHER_CTX_free(encryptionContext.encrypt_ctx);
        return false;
    }

    // Initialize AES decryption context
    encryptionContext.decrypt_ctx = EVP_CIPHER_CTX_new();
    if (!encryptionContext.decrypt_ctx) {
        std::cerr << "Error: Failed to create decryption context." << std::endl;
        EVP_CIPHER_CTX_free(encryptionContext.encrypt_ctx);
        return false;
    }
    if (EVP_DecryptInit_ex(encryptionContext.decrypt_ctx, EVP_aes_256_cbc(), NULL, encryptionContext.sharedKey.data(), encryptionContext.iv) != 1) {
        std::cerr << "Error: Failed to initialize decryption context." << std::endl;
        EVP_CIPHER_CTX_free(encryptionContext.encrypt_ctx);
        EVP_CIPHER_CTX_free(encryptionContext.decrypt_ctx);
        return false;
    }
    
    return true;
}


//function to be called by server and client upon creation of a connection. They will use Kyber 
//key generation to arrive at a secret shared key to use later for AES encrypted communication
// the socket classes must have opposite initiator values upon calling the function
void Socket::setupEncryption()
{
  // seed variables required for keypair generation
  std::vector<uint8_t> d(SEED_LEN, 0);
  std::vector<uint8_t> z(SEED_LEN, 0);
  auto _d = std::span<uint8_t, SEED_LEN>(d);
  auto _z = std::span<uint8_t, SEED_LEN>(z);

  // public/ private keypair variables
  std::vector<uint8_t> pkey(kyber1024_kem::PKEY_LEN, 0);
  std::vector<uint8_t> skey(kyber1024_kem::SKEY_LEN, 0);
  auto _pkey = std::span<uint8_t, kyber1024_kem::PKEY_LEN>(pkey);
  auto _skey = std::span<uint8_t, kyber1024_kem::SKEY_LEN>(skey);

  // cipher variable required shared key generation
  std::vector<uint8_t> cipher(kyber1024_kem::CIPHER_LEN, 0);
  auto _cipher = std::span<uint8_t, kyber1024_kem::CIPHER_LEN>(cipher);

  // shared key variable
  std::vector<uint8_t> shrd_key(KEY_LEN, 0);
  auto _shrd_key = std::span<uint8_t, KEY_LEN>(shrd_key);

  // pseudo-randomness source
  prng::prng_t prng;

  // fill up keygen seeds using PRNG
  prng.read(_d);
  prng.read(_z);

  // generate a keypair
  kyber1024_kem::keygen(_d, _z, _pkey, _skey);

  //run the kyber KEM 
  if (initiator)
  {
    //declare variable to store communicating parties public key
    std::vector<uint8_t> cp_pkey(kyber1024_kem::PKEY_LEN, 0);
    auto _cp_pkey = std::span<uint8_t, kyber1024_kem::PKEY_LEN>(cp_pkey);
    
    //assert(sendKeyData(pkey.data(), _pkey.size()));
    assert(getKeyData(_cp_pkey.data(), _cp_pkey.size()));

    // fill up seed required for key encapsulation, using PRNG
    std::vector<uint8_t> m(SEED_LEN, 0);
    auto _m = std::span<uint8_t, SEED_LEN>(m);
    prng.read(_m);
    
    // encapsulate cipher using communicating parties public key, compute cipher text and obtain KDF
    auto skdf = kyber1024_kem::encapsulate(_m, _cp_pkey, _cipher);

    //send cipher to communicating party
    assert(sendKeyData(_cipher.data(), _cipher.size()));

    //obtain shared key
    skdf.squeeze(_shrd_key);
    
    //generate iv (initialization vector)
    assert (RAND_bytes(encryptionContext.iv, AES_BLOCK_SIZE) == 1);

    // Send IV to the other socket
    assert(sendKeyData(encryptionContext.iv, AES_BLOCK_SIZE));

    #if VERBOSE
    {
        using namespace kyber_utils;

        std::cout << "kyber1024 KEM Results from setupEncryption(): \n";
        std::cout << "\ninitiator       : true";
        std::cout << "\npartner's pubkey: " << to_hex(_cp_pkey);
        std::cout << "\ncipher          : " << to_hex(_cipher);
        std::cout << "\nshared secret   : " << to_hex(_shrd_key);
        std::cout << "\niv              : " << to_hex(encryptionContext.iv) << "\n";
    }
    #endif
  }
  else
  {  
    //assert(getKeyData(_cp_pkey.data(), _cp_pkey.size()));
    assert(sendKeyData(pkey.data(), _pkey.size()));
    
    //get encapsulated cipher
    assert(getKeyData(_cipher.data(), _cipher.size()));

    // decapsulate cipher text and obtain KDF
    auto rkdf = kyber1024_kem::decapsulate(_skey, _cipher);
    
    //obtain shared key
    rkdf.squeeze(_shrd_key);

    //get iv
    assert( getKeyData(encryptionContext.iv, AES_BLOCK_SIZE) );

    #if VERBOSE
    {
        using namespace kyber_utils;

        std::cout << "kyber1024 KEM Results from setupEncryption(): \n";
        std::cout << "\ninitiator     : false";
        std::cout << "\npubkey        : " << to_hex(_pkey);
        std::cout << "\nseckey        : " << to_hex(_skey);
        std::cout << "\ncipher        : " << to_hex(_cipher);
        std::cout << "\nshared secret : " << to_hex(_shrd_key);
        std::cout << "\niv            : " << to_hex(encryptionContext.iv) << "\n";
    }
    #endif
  }
  
  //store the key in class variable for later use
  encryptionContext.sharedKey.assign(shrd_key.begin(), shrd_key.end());

     assert(initAES());    
}

//turns encryption on / off at runtime
void Socket::setCryptography(bool cryptography)
{
    //enable/disable encryption/decryption fucntions
    applyCryptography = cryptography;
}

//frees memory used by encryption context
void Socket::freeEncryptionContext() 
{
    if (encryptionContext.encrypt_ctx != nullptr) 
    {
        EVP_CIPHER_CTX_free(encryptionContext.encrypt_ctx);
        encryptionContext.encrypt_ctx = nullptr; // Set to nullptr to avoid dangling pointers
    }
    if (encryptionContext.decrypt_ctx != nullptr) 
    {
        EVP_CIPHER_CTX_free(encryptionContext.decrypt_ctx);
        encryptionContext.decrypt_ctx = nullptr; // Set to nullptr to avoid dangling pointers
    }

    #if VERBOSE
    cout << "Encryption Context freed" << endl;
    #endif
}

#endif //CRYPTOGRAPHY

#endif /* SOCKET_H */
