# JAM-IM
Simple C++ Console Instant Messenger application based on the Boost/ASIO library.

# Build instructions
$ mkdir build  
$ cd build  
$ cmake ..  
$ make  

# Tests
Uses googletest library for testing. Run tests with ctest.

# Usage
Run Server with:  
$ ./Server <port1> <port2>  

Run Client(s) with:  
$ ./Client <IP> <port1> <port2>  
