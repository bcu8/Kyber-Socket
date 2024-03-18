# Kyber Socket

The Socket class provides socket functionality for encrypted network communication in C++. The code is designed to compile automatically on Linux or Windows through the use of compiler directives. Additional compilation options are provided in the Socket.h file for enabling cryptography and verbose output. 

## Description

The Socket class is subclassed into Server and Client subclasses. In the constructor of either class, the initial operations involved in the process of creating the socket are handled using the respective constructor input variables. See the demonstration code for more details about how to instantiate these classes.

The encryption is AES256 which relies on a PQC (post quantum cryptography) algorithm called Kyber1024 to generate a symmetric key for both communicating sockets. The key is securely transferred over the network in a process that looks like this.
1. Alice generates a public and private keypair 
2. Alice sends Bob her public key
3. Bob generates a new 32 byte shared key
4. Bob encapsulates the shared key using Alices public key and sends it to Alice
5. Alice decapsulates the shared key using her private key
6. Now both parties have a shared secret 32 byte key which can be used in AES256 

After this process, encrypt and decrypt functions are used on all outgoing and incoming messages into the socket until disabled by the setCryptography() function.

## Getting Started

### Dependencies
If you dont want the cryptography functionality, you can set CRYPTOGRAPHY = false in the Socket.h file and ignore these steps. Otherwise..
* You may need to upgrade your g++ compiler to suport C++20 (needed for key generation)
	* For Windows the easiest way I know to do this is demonstrated [here](https://www.youtube.com/watch?v=BzuxGrjMDlI&ab_channel=DeepBhuinya) 
* You will need to install OpenSSL (needed for AES256)
	* For Windows installation it is recommended you use chocolatey a demonstration can be found [here](https://www.youtube.com/watch?v=DadOJjiX_IA&ab_channel=Monir) 
* Socket.h uses code from a repository created by [@itzmeanjan](https://github.com/itzmeanjan) which can be found here [here.](https://github.com/itzmeanjan/kyber)  The code is used to implement the Kyber1024 KEM. All necessary code is included in the kyber directory of this repository so as long as that directory is not changed, no further action is required by you.

### Usage

* Using the Socket class can be as easy as writing "#include Socket.h" in your header file (as shown in the demonstration header files)
* To build you will likely need a Makefile, I have included an example Makefile for Windows in this repo but you might need to change the paths to work with your own installation of OpenSSL

## Authors

* Brandon Udall [@bcu8](https://github.com/bcu8)

## License

This project is licensed under the CC0 1.0 Universal License - see the LICENSE.md file for details

## Acknowledgments

* [kyber-lib](https://github.com/itzmeanjan/kyber)
