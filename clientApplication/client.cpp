#include "client.h"
#include "qdebug.h"

Client::Client() {
    this->hostIP = "127.0.0.1";
    this->portNumber = 5555;
    WSAStartup(MAKEWORD(2, 0), &this->wsaData);
    this->server = socket(AF_INET, SOCK_STREAM, 0);
    inet_pton4(this->hostIP, reinterpret_cast<char *>(&address.sin_addr.s_addr));
    this->address.sin_family = AF_INET;
    this->address.sin_port = htons(this->portNumber);
}

void Client::connectToServer() {
    if (connect(this->server, reinterpret_cast<SOCKADDR *>(&this->address), sizeof(this->address)) == 0){
        this->isConnected = true;
    }
}

void Client::run() {
    this->connectToServer();
}

void Client::closeSocket() {
    closesocket(this->server);
    WSACleanup();
    std::cout << "Disconnected From Server" << std::endl;
}

std::string Client::Receive() {
    char buffer[1024];
    std::string message;
    int n = recv(this->server, buffer, sizeof(buffer), 0);
    message.append(buffer, buffer + n);
    return message;
}

void Client::Send(std::string message) const {
    send(this->server, message.data(), message.size(), 0);
}

std::string Client::constructMessage(std::initializer_list<std::string> il){
    std::string message;
    for (auto const& x: il)
        message += (x + "|");
    return message;
}


std::vector<std::string> Client::deconstructMessage(std::string message){

    std::vector<std::string> m;
    std::string delim = "|";
    auto start = 0U;
    auto end = message.find(delim);
    while (end != std::string::npos){
        m.emplace_back(message.substr(start, end - start));
        start = end + delim.length();
        end = message.find(delim, start);
    }
    m.emplace_back(message.substr(start, end - start));
    return m;
}














