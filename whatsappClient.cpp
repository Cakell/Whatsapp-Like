#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include "whatsappio.h"

using namespace std;


/**
 * The program's valid number of arguments.
 */
#define CLIENT_NUM_OF_ARGS 4

/**
 * The exit code in case of a success.
 */
#define SUCCESS 0

/**
 * The exit code in case of a failure.
 */
#define FAILURE 1

/**
 * The index of the client name in 'argv'.
 */
#define CLIENT_NAME_INDEX 1

/**
 * The index of the server address in 'argv'.
 */
#define SERVER_ADDRESS_INDEX 2

/**
 * The index of the port number in 'argv'.
 */
#define PORT_NUM_INDEX 3

/**
 * The base that is normally used to represent a number - used in 'strtol' function.
 */
#define DECIMAL_BASE 10

/**
 * When this constant is used as the third argument of socket, the default protocol
 * will be chosen - which in our case will be TCP, since we use SOCK_STREAM.
 */
#define DEFAULT_PROTOCOL 0

/**
 * The string that is sent to a client who tries to connect
 * with a name that is already in use in the server.
 */
#define DUP_CONNECTION "dupConnection"

/**
 * The create group message that is sent from a client to the server - which in turn
 * should create a group with the groupName and clientName sent by the client, in case
 * the group creation request is valid.
 */
#define CREATE_GROUP_MSG "create_group"

/**
 * The send message that is sent from a client to the server - which in turn
 * should send the message the to given client or group.
 */
#define SEND_MSG "send"

/**
 * The who message that is sent from a client to the server - which in turn
 * should send back to the client a list of currently connected client name
 * (alphabetically order), seperated by comma without spaces.
 */
#define WHO_MSG "who"

/**
 * The exit message that is sent from a client to the server - which in turn
 * should unregister the client from the server.
 */
#define EXIT_MSG "exit"

/**
 * The exit message that is sent from the server to a client - which informs
 * the client it should exit (with exit code 1 (FAILURE) ).
 */
#define SERVER_EXIT "serverEXIT"


// Global Variables:
string clientName;
int communicationSocketFD;



void freeResources() {
	close(communicationSocketFD);
}

bool nameIsAlphaNumeric(const string& name) {
	for (const char &ch : name) {
		if (! ((bool) isalnum(ch))) {
			return false;
		}
	}
	return true;
}

bool sendClientNameToServer(string &clientName) {
	bool toExit = false;
	writeData(communicationSocketFD, clientName);
	string serverResponse = readData(communicationSocketFD);
	if (serverResponse == DUP_CONNECTION) {
		print_dup_connection();
		freeResources();
		toExit = true;
	} else {
		print_connection();
	}
	return toExit;
}

/*
 * Description: Checks that clients contains at least one name other than us (this client).
 * Also, checks that groupName and all client names in clients are alphanumeric.
*/
bool isGroupValid(string& groupName, vector<string>& clients) {
	if (!nameIsAlphaNumeric(groupName)) {
        return false;
    }
    if (clients.empty()) {
		return false;
	}
	bool isValid = false;
	for (const string &client : clients) {
		if (!nameIsAlphaNumeric(client)) {
			return false;
		}
        if (client == groupName) {
            return false;
        }
		if (client != clientName) {
			isValid = true;
		}
	}
	return isValid;
}

void createGroupCommand(string& groupName, vector<string>& clients) {
	if (!isGroupValid(groupName, clients)) {
		print_create_group(false, false, clientName, groupName);
		return;
	}
	string messageToServer(CREATE_GROUP_MSG);
	messageToServer += (" " + groupName + " ");
	for (const string& clientName : clients) {
		messageToServer += (clientName + ",");
	}
	messageToServer.pop_back();     // deletes last redundant comma.
	writeData(communicationSocketFD, messageToServer);
	string serverResponse = readData(communicationSocketFD);
	print_create_group(false, serverResponse == (to_string(SUCCESS)), clientName, groupName);
}

void sendCommand(string& name, string& message) {
	if ((!nameIsAlphaNumeric(name)) || (name == clientName)) {
		print_send(false, true, false, clientName, name, message);
		return;
	}
	string messageToServer(SEND_MSG);
	messageToServer += (" " + name + " " + message);
	writeData(communicationSocketFD, messageToServer);
	string serverResponse = readData(communicationSocketFD);
	print_send(false, true, serverResponse == (to_string(SUCCESS)), clientName, name, message);
}

void whoCommand() {
	string messageToServer = WHO_MSG;
	writeData(communicationSocketFD, messageToServer);
	string connectedClients = readData(communicationSocketFD);
	print_who_client(connectedClients);
}

bool exitCommand() {
	string messageToServer = EXIT_MSG;
	writeData(communicationSocketFD, messageToServer);
	freeResources();
	print_exit(false, clientName);
	return true;
}

bool sendCommandsToServer() {
	bool toExit = false;
	string clientInput, name, message;
	command_type commandType;
	vector<string> clients;
	clients.clear();

	getline(cin, clientInput);
    if (clientInput.empty()) {
        return toExit;
    }
	parse_command(clientInput, commandType, name, message, clients);
	if (commandType == CREATE_GROUP) {
		createGroupCommand(name, clients);
	} else if (commandType == SEND) {
		sendCommand(name, message);
	} else if (commandType == WHO) {
		whoCommand();
	} else if (commandType == EXIT) {
		toExit = exitCommand();
	} else if (commandType == INVALID) {
		print_invalid_input();
	}
	return toExit;
}

bool handleInputFromServer() {
	bool toExit = false;
	string serverMessage = readData(communicationSocketFD);
	if (serverMessage == SERVER_EXIT) {
		freeResources();
		toExit = true;
	} else {    // it means a message was sent from another client through the server.
		string senderClientName, message;
		command_type commandType;
		vector<string> clients;
		clients.clear();
		// According to the communication protocol between the server and the client,
		// the commandType is necessarily "send", when 'name' is the name of the
		// message sender, and 'message' is the content of the message.
		parse_command(serverMessage, commandType, senderClientName, message, clients);

		// "" is given as the (destination) 'name' arg, which is irrelevant for this print.
		print_send(false, false, true, senderClientName, "", message);
	}
	return toExit;
}


int main(int argc, char *argv[]) {

	clientName = argv[CLIENT_NAME_INDEX];

	if (argc != CLIENT_NUM_OF_ARGS ||
		(!nameIsAlphaNumeric(clientName = argv[CLIENT_NAME_INDEX]))) {
		print_client_usage();
		return FAILURE;
	}

	struct sockaddr_in clientSocketAddress = {0};
	struct hostent *hostEntry;
	bool toExit;
	fd_set serverAndStdInputFdSet, readyToReadFdSet;

	hostEntry = gethostbyname(argv[SERVER_ADDRESS_INDEX]);
	if (hostEntry == nullptr) {
		print_error("gethostbyname", h_errno);
		return FAILURE;
	}
	auto portNum = (unsigned short) strtol(argv[PORT_NUM_INDEX], nullptr, DECIMAL_BASE);

	memset(&clientSocketAddress, 0, sizeof(clientSocketAddress));
	memcpy(&clientSocketAddress.sin_addr, hostEntry->h_addr,
		   (unsigned short) hostEntry->h_length);
	clientSocketAddress.sin_family = (unsigned short) hostEntry->h_addrtype;
	clientSocketAddress.sin_port = htons(portNum);

	// We use TCP, and therefore we use SOCK_STREAM
	communicationSocketFD = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);
	if (communicationSocketFD < 0) {
		print_error("socket", errno);
		return FAILURE;
	}

	if ( connect(communicationSocketFD, (struct sockaddr*) &clientSocketAddress,
				 sizeof(clientSocketAddress)) < 0) {
		close(communicationSocketFD);
		print_fail_connection();
		print_error("connect", errno);
		return FAILURE;
	}

	toExit = sendClientNameToServer(clientName);
	if (toExit) {
		return FAILURE; // when client name is already in use.
	}

	FD_ZERO(&serverAndStdInputFdSet);
	FD_SET(STDIN_FILENO, &serverAndStdInputFdSet);
	FD_SET(communicationSocketFD, &serverAndStdInputFdSet);
	FD_ZERO(&readyToReadFdSet);

	while (!toExit) {
		readyToReadFdSet = serverAndStdInputFdSet;

		if ( select(communicationSocketFD + 1,
					&readyToReadFdSet, nullptr, nullptr, nullptr) < 0) {
			print_error("select", errno);
			freeResources();
			return FAILURE;

		}
		if (FD_ISSET(STDIN_FILENO, &readyToReadFdSet)) {
			toExit = sendCommandsToServer();
			if (toExit) {
				return SUCCESS; // when client exited before server.
			}
		}
		if (FD_ISSET(communicationSocketFD, &readyToReadFdSet)) {
			toExit = handleInputFromServer();
			if (toExit) {
				return FAILURE; // when server exited before client.
			}
		}
	}
	return SUCCESS;
}
