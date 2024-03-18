/*********************************************************************************
 * File: Socket.h
 * Author: Brandon Udall 
 * Created: 3/12/2024
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
 * This code is provided under the Creative Commons CC0 1.0 Universal license.
 * You are free to use, modify, and distribute this code for any purpose without
 * any restrictions. However, the original copyright and license information
 * included in this header comment section must not be removed from the code.
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
#define CRYPTOGRAPHY true 

//comprehensive debugging information will be printed to terminal
#define VERBOSE false

/************************************************************************
 * Headers
 ************************************************************************/

//win/linux std libs
#include <iostream>
#include <string>
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

#ifdef _WIN32 //only include on windows
    #include <Winsock2.h>
    #include <ws2tcpip.h>
#elif defined(__linux__) //only include on linux
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

#if CRYPTOGRAPHY
constexpr size_t SEED_LEN = 32;
constexpr size_t KEY_LEN = 32;
#endif

#define MAX_MSG_LEN 500

/************************************************************************
 * Socket class declaration
 ************************************************************************/

//socket is the parent of server and client subclasses. You cant declare a Socket class directly.
//you must use one of the subclasses. Use alternate Server() constructor in the case of a server 
//accepting client connections and passing them to threads... other than that its self explanitory (call server for server, client for client)
class Socket {
protected: //protected methods and variables can be accessed by the Socket class and its subclasses but not outside of these classes
    Socket() {}
    virtual ~Socket() {}

    #if CRYPTOGRAPHY
    // Define the encryption context structure
    struct CryptographyContext {
        EVP_CIPHER_CTX *encrypt_ctx;
        EVP_CIPHER_CTX *decrypt_ctx;
        std::vector<uint8_t> sharedKey;
        unsigned char iv[AES_BLOCK_SIZE];
    };

    //socket class encryption variables (available to client and server subclasses)
    bool initiator;
    bool applyCryptography;
    CryptographyContext encryptionContext;

    //socket class encryption methods (available to client and server subclasses)
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
    //socket class variables (available to client and server subclasses)
    int socketId;
    bool autoPrintResponses;

    //socket class methods (available to client and server subclasses)
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
    //client constructor attempts to connect to server on given port
    //auto print allows getString() and related get methods to print incoming messages to terminal automatically
    Client(const string serverIp, const int port, const bool autoPrint);

    //destructor
    ~Client();
};

/************************************************************************
 * Server sub-class declaration
 ************************************************************************/

class Server : public Socket {
public:
    //standard server constructor meant to accept client connections then create threads to handle connection
    Server(const int port, const int numServerThreads, const bool autoPrint, const bool portReuse);
    //alternate Server constructor (for server threads)
    Server( const int socket, const bool autoPrint);
    //destructor
    ~Server();
    //prevents wait time when restarting server on same port
    void allowPortReuse();
    //wait for and accept client connections
    bool acceptConnection(int *client_socket);
};

/************************************************************************
 * Socket Methods (available to both Client and Server subclasses)
 ************************************************************************/

//waits for string from connected socket. expects length of string then string (as done in sendString())
inline bool Socket::getString(string& str) {
    // Clear string before using it
    str.clear();
    //pre-set result in case cryptography is disabled
    bool result = true;

    // Read the length of the incoming string
    int length;
    if (recv(socketId, reinterpret_cast<char*>(&length), sizeof(length), 0) <= 0) {
        return false; // Connection disruption or error
    }

    // Resize the string to accommodate the incoming data
    str.resize(length);

    // Receive the data
    int bytesReceived = recv(socketId, &str[0], length, 0);
    if (bytesReceived <= 0) {
        return false; // Connection disruption or error
    }

    #if CRYPTOGRAPHY
    // Decrypt the message
    result = decrypt(str);
    #endif

    // Print if autoPrintResponses is true
    if (autoPrintResponses) {
        cout << str << endl;
    }

    //return success / fail of decrypt
    return result;
}

//send given string to connected socket. sends length of string followed by string
inline bool Socket::sendString(string str) {
    bool result = true;

    #if CRYPTOGRAPHY
    // Encrypt the string
    result = encrypt(str);
    #endif

    // Send the length of the string first
    int length = str.length();
    if (send(socketId, reinterpret_cast<const char*>(&length), sizeof(length), 0) != sizeof(length)) {
        return false; // Error in sending length
    }

    // Send the data
    int sendResult = send(socketId, str.c_str(), length, 0);
    if (sendResult != length) {
        return false; // Error in sending data
    }

    return result;
}

/************************************************************************
 * Client Class Methods
 ************************************************************************/

//constructor, attempts to establish connection to given server on given port
//if fail, runtime exception is thrown. Uses compiler directives to implement
//windows or linux sockets based on the platform the caller is using
Client::Client(const string serverIp, const int port, const bool autoPrint)
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
        initiator = false; //must be opposite of the initiator value of server
        applyCryptography = true; //start with encryption / decryption enabled

        //collaborate with connected party to get shared encryption key
        setupEncryption();
    #endif
}

// Destructor
Client::~Client() {
    #if CRYPTOGRAPHY
        //free cryptography resources
        freeEncryptionContext();
    #endif
    
    // Close the client socket
    #ifdef _WIN32
        closesocket(socketId);
        WSACleanup();
    #elif defined(__linux__)
        close(socketId);
    #endif
}

/************************************************************************
 * Server Class Methods
 ************************************************************************/

// Constructor, creates socket, binds to port, then listens for incoming connections. 
// connections can be accepted with acceptConnection()
Server::Server(const int port, const int numServerThreads, bool autoPrint, bool portReuse) 
{   
    autoPrintResponses = autoPrint;
    
    #if CRYPTOGRAPHY
    applyCryptography = true; //start with cryptography enabled
    initiator = true; //differs from client
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

//Alternate constructor, use this one when you make a thread for a client connection
Server::Server( const int socket, const bool autoPrint)
{
    socketId = socket;
    autoPrintResponses = autoPrint;

    #if CRYPTOGRAPHY
        initiator = true;
        applyCryptography = true;
    
        //collaborate with connected party to get shared encryption key
        setupEncryption();
    #endif
}

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

//destructor
Server::~Server() {
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

    unsigned char cipherText[MAX_MSG_LEN]; 
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

    unsigned char plainText[MAX_MSG_LEN]; 
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

//initializes the encryption and decryption contexts
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
    
    //get public key from communicating party
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
    //send public key to communicating party
    assert(sendKeyData(pkey.data(), _pkey.size()));
    
    //get encapsulated cipher
    assert(getKeyData(_cipher.data(), _cipher.size()));

    // decapsulate cipher text and obtain KDF
    auto rkdf = kyber1024_kem::decapsulate(_skey, _cipher);
    
    //obtain shared key
    rkdf.squeeze(_shrd_key);

    //get iv from communicating party
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

     //initialize AES encryption and decryption contexts
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
