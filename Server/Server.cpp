#include "Server.h"
#include <exception>
#include <iostream>
#include <string>
#include <thread>

#define CHAT_PADDING_LEN 5
#define USERNAMES_PADDING_LEN 5
#define SECOND_USERNAME_PADDING_LEN 2

Server::Server()
{

	// this server use TCP. that why SOCK_STREAM & IPPROTO_TCP
	// if the server use UDP we will use: SOCK_DGRAM & IPPROTO_UDP
	_serverSocket = socket(AF_INET,  SOCK_STREAM,  IPPROTO_TCP); 

	if (_serverSocket == INVALID_SOCKET)
		throw std::exception(__FUNCTION__ " - socket");
}

Server::~Server()
{
	try
	{
		// the only use of the destructor should be for freeing 
		// resources that was allocated in the constructor
		closesocket(_serverSocket);
	}
	catch (...) {}
}

void Server::serve(int port)
{
	
	struct sockaddr_in sa = { 0 };
	
	sa.sin_port = htons(port); // port that server will listen for
	sa.sin_family = AF_INET;   // must be AF_INET
	sa.sin_addr.s_addr = INADDR_ANY;    // when there are few ip's for the machine. We will use always "INADDR_ANY"

	// Connects between the socket and the configuration (port and etc..)
	if (bind(_serverSocket, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - bind");
	
	// Start listening for incoming requests of clients
	if (listen(_serverSocket, SOMAXCONN) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - listen");
	std::cout << "Listening on port " << port << std::endl;

	while (true)
	{
		// the main thread is only accepting clients 
		// and add then to the list of handlers
		std::cout << "Waiting for client connection request" << std::endl;
		acceptClient();
	}
}

std::string Server::loginServerUpdateMessage(SOCKET clientSocket)
{
	int len_username = Helper::getIntPartFromSocket(clientSocket, SECOND_USERNAME_PADDING_LEN);//gets the length of the username
	std::string userName = Helper::getStringPartFromSocket(clientSocket, len_username);//gets the username from the socket
	
	/*auto it = std::find(this->_usernames.begin(), this->_usernames.end(), userName);
	if (it != this->_usernames.end() || this->_usernames.empty())
	{
		this->_usernames.push_back(userName);//adds the user to the vector of users
		Helper::send_update_message_to_client(clientSocket, "", "", getUsers());//returns the return message of the server to the login
		return userName;//returns the username so the thread of the user would have its name
	}
	else {
		throw std::exception("the name already exists\n");
	}*/

	this->_usernames.push_back(userName);//adds the user to the vector of users
	Helper::send_update_message_to_client(clientSocket, "", "", getUsers());//returns the return message of the server to the login
	return userName;//returns the username so the thread of the user would have its name
}

void Server::acceptClient()
{

	// this accepts the client and create a specific socket from server to this client
	// the process will not continue until a client connects to the server
	SOCKET client_socket = accept(_serverSocket, NULL, NULL);
	if (client_socket == INVALID_SOCKET)
		throw std::exception(__FUNCTION__);

	std::cout << "Client accepted. Server and client can speak" << std::endl;
	// the function that handle the conversation with the client
	std::thread cleint_thread(&Server::clientHandler, this ,client_socket);
	cleint_thread.detach();
}


void Server::clientHandler(SOCKET clientSocket)
{
	std::string username = "";
	while (true)
	{
		try
		{
			int CODE = Helper::getMessageTypeCode(clientSocket);

			if (CODE == 200)
			{
				//adds the username to the map of usernames and returns the server message with code 101
				username = loginServerUpdateMessage(clientSocket);
			}
			else if (CODE == 204) {
				if (username != "")
				{
					//writes the message to the file of the text messages of the two users
					addMessage(clientSocket, username);
				}
			}
		}
		catch (const std::exception& e)
		{
			closesocket(clientSocket);
			std::cout << username << " disconnected!!!\n";
			auto it = std::find(_usernames.begin(), _usernames.end(), username);
			_usernames.erase(it);
			return;
		}
	}
}


std::string Server::fileToStr(std::string fileName)
{
	if (fileName == "")return "";
	std::unique_lock<std::mutex> lock(_messages_lock);

	std::ifstream dataFile;
	dataFile.open(fileName);

	if (!dataFile.is_open())
	{
		std::cout << "couldn't open the file or create it\n";
		dataFile.close();
		lock.unlock();
		return "";
	}

	std::stringstream text;

	text << dataFile.rdbuf();

	dataFile.close();
	lock.unlock();
	return text.str();
}

std::string Server::getFileName(std::string name1, std::string name2)
{
	if (name1 == "" || name2 == "")return "";

	if (name1 < name2)
	{
		return(name1 + "&" + name2 + ".txt");
	}
	else {
		return(name2 + "&" + name1 + ".txt");
	}
}

void Server::addMessage(SOCKET clientSocket, std::string senderUsername)
{
	bool isNewMessage = true;
	std::string fileContent = "";

	//gets the username of the reciever of the message
	int len_second_user = Helper::getIntPartFromSocket(clientSocket, SECOND_USERNAME_PADDING_LEN);
	std::string second_username = Helper::getStringPartFromSocket(clientSocket, len_second_user);

	//gets the message to the second user
	int len_new_message = Helper::getIntPartFromSocket(clientSocket, CHAT_PADDING_LEN);
	std::string new_message = Helper::getStringPartFromSocket(clientSocket, len_new_message);

	if (new_message == "")
	{
		isNewMessage = false;
	}
	std::string fileName = getFileName(senderUsername, second_username);//gets the name file;
	if (isNewMessage)
	{
		addMessageToFile(new_message, senderUsername, fileName);//writes the new message to the file
	}

	fileContent = fileToStr(fileName);//gets the file content as a string
	Helper::send_update_message_to_client(clientSocket, fileContent, second_username, getUsers());//returns the server message to the client
}

std::string Server::getUsers()
{
	std::string result = "";
	
	for (auto it = _usernames.begin();it != _usernames.end();it++)
	{
		if (it == _usernames.begin())
		{
			result += *it;
		}
		else {
			result += "&" + *it;
		}
	}

	return result;
}

void Server::addMessageToFile(std::string message, std::string senderUserName, std::string fileName)
{
	std::string msg = "&MAGSH_MESSAGE&&Author&"  + senderUserName + "&DATA&" + message;//file writing format

	std::fstream dataFile(fileName, std::ios::app);

	if (!dataFile.is_open())
	{
		std::cerr << "couldn't open the file or create it\n";
		return;
	}

	dataFile << msg;
	dataFile.close();
}
