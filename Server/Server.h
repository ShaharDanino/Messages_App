#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <queue>
#include <string>
#include <mutex>
#include "Helper.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>


class Server
{
public:
	Server();
	~Server();
	void serve(int port);
	//void deletePointers();
	
	
private:
	//std::queue<std::string> _server_messages;
	std::mutex _messages_lock;
	std::vector<std::string> _usernames;//username and is connected
	//std::queue<std::unique_ptr<std::ofstream>> _filePointers;

	std::string loginServerUpdateMessage(SOCKET clientSocket);
	void acceptClient();
	void clientHandler(SOCKET clientSocket);
	std::string fileToStr(std::string fileName);
	std::string getFileName(std::string name1, std::string name2);
	void addMessage(SOCKET clientSocket, std::string senderUsername);
	std::string getUsers();
	void addMessageToFile(std::string message, std::string senderUserName, std::string fileName);

	SOCKET _serverSocket;
};

