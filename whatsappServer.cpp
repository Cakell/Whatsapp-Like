#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <set>
#include <map>
#include "whatsappio.h"

using namespace std;


/**
 * The program's valid number of arguments.
 */
#define SERVER_NUM_OF_ARGS 2

/**
 * The exit command from the standard input, telling the server it should terminate.
 */
#define EXIT_COMMAND "EXIT"


/**
 * The exit code in case of a success.
 * Also used as a success message that is sent to a client after a successful read.
 */
#define SUCCESS 0

/**
 * The exit code in case of a failure.
 */
#define FAILURE 1

/**
 * The index of the port number in 'argv'.
 */
#define PORT_NUM_INDEX 1

/**
 * The base that is normally used to represent a number - used in 'strtol' function.
 */
#define DECIMAL_BASE 10

/**
 * The maximal length of a host name.
 */
#define MAX_HOST_NAME_LENGTH 256

/**
 * When this constant is used as the third argument of socket, the default protocol
 * will be chosen - which in our case will be TCP, since we use SOCK_STREAM.
 */
#define DEFAULT_PROTOCOL 0

/**
 * The maximal number of pending connections in the socket's listen queue.
 */
#define MAX_NUM_OF_CLIENTS 10

/**
 * The string that is sent to a client who tries to connect
 * with a name that is already in use in the server.
 */
#define DUP_CONNECTION "dupConnection"

/**
 * The exit message that is sent from the server to a client - which informs
 * the client it should exit (with exit code 1 (FAILURE) ).
 */
#define SERVER_EXIT "serverEXIT"



// global Variables:
static map<int, string> fdToClientName;         // Maps clientFDs to their name
static map<string, int> clientNameToFd;         // Maps client names to their fd.
static map<string, set<string>> groups;         // Maps group names to their participants
static set<int> clientsFileDescriptors;
static set<int> allFileDescriptors;
static set<string> clientNames;
fd_set allFDsSet, readyToReadFdSet;
int listeningSocketFD;



void connectNewClient(int clientSocketFD) {
	string clientName = readData(clientSocketFD);
	if ((clientNames.count(clientName) > 0) ||
			(groups.find(clientName) != groups.end())) {  //i.e. if clientName is already in use:
		string response = DUP_CONNECTION;
		writeData(clientSocketFD, response);
	} else {
		FD_SET(clientSocketFD, &allFDsSet);
		clientsFileDescriptors.insert(clientSocketFD);
		allFileDescriptors.insert(clientSocketFD);
		clientNames.insert(clientName);
		fdToClientName[clientSocketFD] = clientName;
		clientNameToFd[clientName] = clientSocketFD;
		string response = to_string(SUCCESS);
		writeData(clientSocketFD, response);
		print_connection_server(clientName);
	}
}


bool serverStdInput() {
	bool toExit = false;
	string userInput;
	getline(cin, userInput);
	if (userInput == EXIT_COMMAND) {
		toExit = true;
		print_exit();
		close(listeningSocketFD);
		for (const int &clientFileDescriptor: clientsFileDescriptors) {
			string serverExit(SERVER_EXIT);
			writeData(clientFileDescriptor, serverExit);
			close(clientFileDescriptor);
		}
	}
	return toExit;

}

bool isGroupValid(string& groupName, vector<string>& clients) {
	if ((clientNames.count(groupName) > 0) || (groups.find(groupName) != groups.end())) {
		// which means the group name is already in use by another group or a client.
		return false;
	}
	for (const string &client : clients) {
		if (clientNames.count(client) == 0) {
			// which means there is no client with that name.
			return false;
		}
	}
	return true;
}


void handleCreateGroupRequest(int clientSocketFD, string& groupName, vector<string>& clients) {
    string response;
    string clientName = fdToClientName[clientSocketFD];
    if (!isGroupValid(groupName, clients)) {
        print_create_group(true, false, clientName, groupName);
        response = to_string(FAILURE);
    } else {
        set<string> uniqueClients;
        for (const string &client : clients) {
            uniqueClients.insert(client);   // makes duplicate members appear once in group.
            uniqueClients.insert(clientName);
        }
		groups[groupName] = uniqueClients;
        print_create_group(true, true, clientName, groupName);
        response = to_string(SUCCESS);
    }
    writeData(clientSocketFD, response);
}


void handleSendRequest(int senderClientFD, string& name, string& message) {
	string responseToSenderClient;
	string senderClientName = fdToClientName[senderClientFD];

	if (clientNames.count(name) > 0) {
		print_send(true, true, true, senderClientName, name, message);
		responseToSenderClient = to_string(SUCCESS);

		int receiverClientFD = clientNameToFd[name];
		string messageToReceiverClient = "send " + senderClientName + " " + message;
		writeData(receiverClientFD, messageToReceiverClient);
	}
	else if (groups.find(name) != groups.end()) {
		set<string> clientsInGroup = groups[name];
		if(clientsInGroup.count(senderClientName) == 0) { // sender is not a member of this group
			print_send(true, true, false, senderClientName, name, message);
			responseToSenderClient = to_string(FAILURE);
		}
		else {  // if sender is a member of this group:
			print_send(true, true, true, senderClientName, name, message);
			responseToSenderClient = to_string(SUCCESS);

			string messageToReceiverClient = "send " + senderClientName + " " + message;
			for (const string &receiverClientName : clientsInGroup)
			{
				if (receiverClientName != senderClientName) {
					int receiverClientFD = clientNameToFd[receiverClientName];
					writeData(receiverClientFD, messageToReceiverClient);
				}
			}
		}
	}
	else {      // i.e. : name is neither a client name nor a group name:
		print_send(true, true, false, senderClientName, name, message);
		responseToSenderClient = to_string(FAILURE);
	}
	writeData(senderClientFD, responseToSenderClient);
}


void handleWhoRequest(int clientSocketFD) {
	string response;
	string clientName = fdToClientName[clientSocketFD];

	print_who_server(clientName);
	for (const string &client : clientNames) {
		response += (client + ",");
	}
	response.pop_back();    // deletes last redundant comma.
	writeData(clientSocketFD, response);
}


void handleExitRequest(int clientSocketFD) {
	string response;
	string clientName = fdToClientName[clientSocketFD];

	clientsFileDescriptors.erase(clientSocketFD);
	allFileDescriptors.erase(clientSocketFD);
	clientNames.erase(clientName);
	fdToClientName.erase(clientSocketFD);
	clientNameToFd.erase(clientName);
    FD_CLR(clientSocketFD, &allFDsSet);
    FD_CLR(clientSocketFD, &readyToReadFdSet);
	// for group in groups: remove clientName from group.
    for (auto &groupParticipantsPair : groups) {
        if (groupParticipantsPair.second.count(clientName) > 0) {
            groupParticipantsPair.second.erase(clientName);
        }
    }
	close(clientSocketFD);
	print_exit(true, clientName);
}


void handleClientRequest(int clientSocketFD) {
    string clientInput, name, message;
    command_type commandType;
    vector<string> clients;
	clients.clear();

    clientInput = readData(clientSocketFD);
    parse_command(clientInput, commandType, name, message, clients);

    if (commandType == CREATE_GROUP) {
        handleCreateGroupRequest(clientSocketFD, name, clients);
    } else if (commandType == SEND) {
		handleSendRequest(clientSocketFD, name, message);
	} else if (commandType == WHO) {
		handleWhoRequest(clientSocketFD);
	} else if (commandType == EXIT) {
		handleExitRequest(clientSocketFD);
	}
}


int main(int argc, char *argv[]) {
	if (argc != SERVER_NUM_OF_ARGS) {
		print_server_usage();
		return FAILURE;
	}

	auto portNum = (unsigned short) strtol(argv[PORT_NUM_INDEX], nullptr, DECIMAL_BASE);
	char myHostName[MAX_HOST_NAME_LENGTH + 1];
	struct sockaddr_in serverSocketAddress = {0};
	struct sockaddr_in clientSocketAddress = {0};
	struct hostent *hostEntry;
	int clientSocketFD;
	bool toExit = false;
	FD_ZERO(&allFDsSet);
	FD_ZERO(&readyToReadFdSet);

	if ( gethostname(myHostName, MAX_HOST_NAME_LENGTH) < 0) {
		print_error("gethostname", errno);
	}
	hostEntry = gethostbyname(myHostName);
	if (hostEntry == nullptr) {
		print_error("gethostbyname", h_errno);
		return FAILURE;
	}

	memset( &serverSocketAddress, 0, sizeof(serverSocketAddress));
	serverSocketAddress.sin_family = (unsigned short) hostEntry->h_addrtype;
	serverSocketAddress.sin_addr.s_addr = htons(INADDR_ANY);
	serverSocketAddress.sin_port = htons(portNum);

	// We use TCP, and therefore we use SOCK_STREAM
	listeningSocketFD = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);
	if (listeningSocketFD < 0) {
		print_error("socket", errno);
		return FAILURE;
	}

	if ( bind( listeningSocketFD, (struct sockaddr*) &serverSocketAddress,
	           sizeof(struct sockaddr_in)) < 0) {
		print_error("bind", errno);
		return FAILURE;
	}

	if ( listen( listeningSocketFD, MAX_NUM_OF_CLIENTS) < 0) {
		print_error("listen", errno);
		return FAILURE;
	}

	socklen_t clientSocketAddressLength = sizeof(clientSocketAddress);
	FD_SET(listeningSocketFD, &allFDsSet);
	FD_SET(STDIN_FILENO, &allFDsSet);
	allFileDescriptors.insert(listeningSocketFD);
	allFileDescriptors.insert(STDIN_FILENO);

	while (!toExit) {
		readyToReadFdSet = allFDsSet;

		if ( select(*allFileDescriptors.rbegin() + 1,
		            &readyToReadFdSet, nullptr, nullptr, nullptr) < 0) {
			print_error("select", errno);
			return FAILURE;
		}
		if (FD_ISSET(listeningSocketFD, &readyToReadFdSet)) {
			clientSocketFD = accept(listeningSocketFD,
			                        (struct sockaddr *) &clientSocketAddress,
			                        &clientSocketAddressLength);
			if (clientSocketFD < 0)
			{
				print_error("accept", errno);
				return FAILURE;
			}
			connectNewClient(clientSocketFD);
		}
		if (FD_ISSET(STDIN_FILENO, &readyToReadFdSet)) {
			toExit = serverStdInput();
		}
		for (const int &clientFileDescriptor: clientsFileDescriptors) {
			if (FD_ISSET(clientFileDescriptor, &readyToReadFdSet)) {
				handleClientRequest(clientFileDescriptor);
                break;
			}
		}
	}
	return SUCCESS;
}