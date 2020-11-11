/* 
 * A simple online stock exchange web-server.  
 * 
 * This multithreaded web-server performs simple stock trading
 * transactions on stocks.  Stocks must be maintained in an
 * unordered_map.
 * Copyright (C) 2020 xiaos3@miamiOH.edu
 */

// The commonly used headers are included.  Of course, you may add any
// additional headers as needed.
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <iomanip>
#include <vector>
#include <fstream>
#include "Stock.h"

// Setup a server socket to accept connections on the socket
using namespace boost::asio;
using namespace boost::asio::ip;

// Shortcut to smart pointer with TcpStream
using TcpStreamPtr = std::shared_ptr<tcp::iostream>;

// Forward declaration for methods defined further below
std::string url_decode(std::string);

// The name space to hold all of the information that is shared
// between multiple threads.
namespace sm {
    // Unordered map including stock's name as the key (std::string)
    // and the actual Stock entry as the value.
    std::unordered_map<std::string, Stock> stockMap;

}  // namespace sm

std::string transCreate(const std::string& s, 
        const std::size_t& found) {
    // If the stock has not present in the unorder map then do the 
    // folling process
    std::string amount = ""; std::string content = ""; 
    std::string stockName = "";
    if (s.find("&", found) == std::string::npos) {
        stockName = s.substr(found + 6);
    } else {
        stockName = s.substr(found + 6, s.find("&", found) - found - 6);
    }
    if (sm::stockMap.find(stockName) == sm::stockMap.end()) {
        if (s.find("&", found) == std::string::npos) {
            sm::stockMap[stockName].name    = stockName;
            sm::stockMap[stockName].balance = 0;
        } else {
            amount    = s.substr(s.find("&", found + 1) + 8);
            sm::stockMap[stockName].name    = stockName;
            sm::stockMap[stockName].balance = std::stoi(amount);
        }
        content = "Stock " + stockName + " created with balance = " +
                    std::to_string(sm::stockMap[stockName].balance);
        return content;
    } else {
        content = "Stock " + stockName + " already exists";
        return content;
    }
}

// Process if trans = status
std::string transStatus(const std::string& s, const std::size_t& found) {
    std::string content = "";
    std::string stockName = "";
    if (s.find("&", found) == std::string::npos) {
        stockName = s.substr(found + 6);
    } else {
        stockName = s.substr(found + 6, s.find("&", found) - found - 6);
    }
    // If the stock is not present in the map
    if (sm::stockMap.find(stockName) == sm::stockMap.end()) {
        content = "Stock not found";
        return content;
    // If the stock is present in the map
    } else {
        // Return with balance
        content = "Balance for stock " + sm::stockMap[stockName].name + 
                " = " + std::to_string(sm::stockMap[stockName].balance);
        return content;
    }
}

// Process if trans = buy
std::string transBuy(const std::string& s,  
        const std::size_t& found) {
    std::string amount = ""; std::string content; 
    std::string stockName = "";
    if (s.find("&", found) == std::string::npos) {
        stockName = s.substr(found + 6);
    } else {
        stockName = s.substr(found + 6, s.find("&", found) - found - 6);
    }
    // If the stock is not present in the map
    if (sm::stockMap.find(stockName) == sm::stockMap.end()) {
        content = "Stock not found";
        return content;
    // If the stock is present in the map
    } else {
        // Read amount from the string
        amount    = s.substr(s.find("&", found + 1) + 8);
        // Use stoi to transfer the string amount to int and minus with balance
        int newAmount = std::stoul(std::to_string
        (sm::stockMap[stockName].balance)) - std::stoul(amount);
        sm::stockMap[stockName].balance = newAmount;
        content = "Stock " + sm::stockMap[stockName].name + "\'s balance "
                "updated";
        return content;
    }
}

// Process if trans = sell
std::string transSell(const std::string& s, 
        const std::size_t& found) {
    std::string amount = ""; std::string content = ""; 
    std::string stockName = "";
    if (s.find("&", found) == std::string::npos) {
        stockName = s.substr(found + 6);
    } else {
        stockName = s.substr(found + 6, s.find("&", found) - found - 6);
    }
    // If the stock is not present in the map
    if (sm::stockMap.find(stockName) == sm::stockMap.end()) {
        content = "Stock not found";
        return content;
    // If the stock is present in the map
    } else {
        // Read amount from the string
        amount    = s.substr(s.find("&", found + 1) + 8);
        // Use stoi to transfer the string amount to int and add up with balance
        sm::stockMap[stockName].balance = sm::stockMap[stockName].balance +
                std::stoul(amount);
        content = "Stock " + sm::stockMap[stockName].name + "\'s balance "
                "updated";
        return content;
    }
}


std::string processStock(const std::string& s) {
    // Separate each section
    std::size_t found = s.find("&") + 1;
    std::string trans = s.substr(6, found - 7); 
    // Process with differen trans command
    if (trans == "create") {
        return transCreate(s, found);
    } else if (trans == "status") {
        return transStatus(s, found);
    } else if (trans == "buy") {
        return transBuy(s, found);
    } else if (trans == "sell") {
        return transSell(s, found);
    }
    return "";
}

// Process data from client
void serverClient(std::istream& is, std::ostream& os) {
    const std::string HTTPRespHeader =
    "HTTP/1.1 200 OK\r\n"
    "Server: localhost\r\n"
    "Connection: Close\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: %1%\r\n\r\n";
    std::string line = ""; std::string words=""; std::string content = "";
    // Read string from client
    while (is >> words) {
        if (words == "HTTP/1.1") {
            break;
        } else if (words == "GET") {
            continue;
        } else {
            line = words.substr(1);
        }
    }
    content = processStock(line);
    auto header = boost::str(boost::format(HTTPRespHeader) % 
    content.length());
    os << header << content << std::endl;
}

/**
 * Top-level method to run a custom HTTP server to process stock trade
 * requests using multiple threads. Each request should be processed
 * using a separate detached thread. This method just loops for-ever.
 *
 * \param[in] server The boost::tcp::acceptor object to be used to accept
 * connections from various clients.
 *
 * \param[in] maxThreads The maximum number of threads that the server
 * should use at any given time.
 */
void runServer(tcp::acceptor& server, const int maxThreads) {
    // Process client connections one-by-one...forever. This method
    // must use a separate detached thread to process each
    // request.
    //
    // Optional feature: Limit number of detached-threads to be <=
    // maxThreads.
    while (true) {
        auto client = 
            std::make_shared<tcp::iostream>();
        server.accept(*client -> rdbuf());
        std::thread thr([client]() {
            serverClient(*client, *client); });
            thr.detach();
    }
}
//-------------------------------------------------------------------
//  DO  NOT   MODIFY  CODE  BELOW  THIS  LINE
//-------------------------------------------------------------------

/** Convenience method to decode HTML/URL encoded strings.
 *
 * This method must be used to decode query string parameters supplied
 * along with GET request.  This method converts URL encoded entities
 * in the from %nn (where 'n' is a hexadecimal digit) to corresponding
 * ASCII characters.
 *
 * \param[in] str The string to be decoded.  If the string does not
 * have any URL encoded characters then this original string is
 * returned.  So it is always safe to call this method!
 *
 * \return The decoded string.
*/
std::string url_decode(std::string str) {
    // Decode entities in the from "%xx"
    size_t pos = 0;
    while ((pos = str.find_first_of("%+", pos)) != std::string::npos) {
        switch (str.at(pos)) {
            case '+': str.replace(pos, 1, " ");
            break;
            case '%': {
                std::string hex = str.substr(pos + 1, 2);
                char ascii = std::stoi(hex, nullptr, 16);
                str.replace(pos, 3, 1, ascii);
            }
        }
        pos++;
    }
    return str;
}

// Helper method for testing.
void checkRunClient(const std::string& port, const bool printResp = false);

/*
 * The main method that performs the basic task of accepting
 * connections from the user and processing each request using
 * multiple threads.
 *
 * \param[in] argc This program accepts up to 2 optional command-line
 * arguments (both are optional)
 *
 * \param[in] argv The actual command-line arguments that are
 * interpreted as:
 *    1. First one is a port number (default is zero)
 *    2. The maximum number of threads to use (default is 20).
 */
int main(int argc, char** argv) {
    // Setup the port number for use by the server
    const int port   = (argc > 1 ? std::stoi(argv[1]) : 0);
    // Setup the maximum number of threads to be used.
    const int maxThr = (argc > 2 ? std::stoi(argv[2]) : 20);

    // Create end point.  If port is zero a random port will be set
    io_service service;    
    tcp::endpoint myEndpoint(tcp::v4(), port);
    tcp::acceptor server(service, myEndpoint);  // create a server socket
    // Print information where the server is operating.    
    std::cout << "Listening for commands on port "
              << server.local_endpoint().port() << std::endl;

    // Check and start tester client for automatic testing.
#ifdef TEST_CLIENT
    checkRunClient(argv[1]);
#endif

    // Run the server on the specified acceptor
    runServer(server, maxThr);
    
    // All done.
    return 0;
}

// End of source code
