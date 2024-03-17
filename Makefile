# Makefile for compiling the program along with Kyber library on Windows

# Compiler
CXX = g++
# Compiler flags
CXXFLAGS = -std=c++20 -Wall -O3 -march=native
# Paths to Kyber headers
KYBER_HEADERS = ./kyber/include
SHA3_HEADERS = ./kyber/sha3/include
SUBTLE_HEADERS = ./kyber/subtle/include
# Path to OpenSSL libraries
OPENSSL_LIB_DIR = "C:/Program Files/OpenSSL-Win64/lib/VC/x64/MDd"
# Path to OpenSSL includes
OPENSSL_INCLUDE_DIR = "C:/Program Files/OpenSSL-Win64/include"
# Source files
SRCS = DemoClient.cpp
# Executable name
TARGET = client.exe

# Rule for compiling .cpp files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I$(KYBER_HEADERS) -I$(SHA3_HEADERS) -I$(SUBTLE_HEADERS) -I$(OPENSSL_INCLUDE_DIR) -c $< -o $@

# Rule for linking object files into executable
$(TARGET): $(SRCS:.cpp=.o)
	$(CXX) $(CXXFLAGS) -o $@ $^ -L$(OPENSSL_LIB_DIR) -lssl -lcrypto -lws2_32

# Declare source files as phony to force recompilation
.PHONY: $(SRCS)

.PHONY: clean

clean:
	del /Q $(TARGET) $(SRCS:.cpp=.o)
