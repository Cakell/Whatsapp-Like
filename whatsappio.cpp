#include "whatsappio.h"
#include <cstdio>
#include <unistd.h>

/**
 * The return value of readData in case of a failure.
 */
#define READ_FAILURE "-1"

/**
 * The return value of writeData in case of a failure.
 */
#define WRITE_FAILURE (-1)

/**
 * The base that is normally used to represent a number - used in 'strtol' function.
 */
#define DECIMAL_BASE 10

/**
 * The length of the string that holds the content of
 * the number of bytes to read from the message.
 */
#define BYTES_TO_READ_LENGTH 4

/*
 * Description: Prints to the screen a message when the user terminate the
 * server
*/
void print_exit() {
    printf("EXIT command is typed: server is shutting down\n");
}

/*
 * Description: Prints to the screen a message when the client established
 * connection to the server, in the client
*/
void print_connection() {
    printf("Connected Successfully.\n");
}

/*
 * Description: Prints to the screen a message when the client established
 * connection to the server, in the server
 * client: Name of the sender
*/
void print_connection_server(const std::string& client) {
    printf("%s connected.\n", client.c_str());
}


/*
 * Description: Prints to the screen a message when the client tries to
 * use a name which is already in use
*/
void print_dup_connection() {
    printf("Client name is already in use.\n");
}

/*
 * Description: Prints to the screen a message when the client fails to
 * establish connection to the server
*/
void print_fail_connection() {
    printf("Failed to connect the server\n");
}

/*
 * Description: Prints to the screen the usage message of the server
*/
void print_server_usage() {
    printf("Usage: whatsappServer portNum\n");
}

/*
 * Description: Prints to the screen the usage message of the client
*/
void print_client_usage() {
    printf("Usage: whatsappClient clientName serverAddress serverPort\n");
}

/*
 * Description: Prints to the screen the messages of "create_group" command
 * server: true for server, false for client
 * success: Whether the operation was successful
 * client: Client name
 * group: Group name
*/
void print_create_group(bool server, bool success,
                        const std::string& client, const std::string& group) {
    if(server) {
        if(success) {
            printf("%s: Group \"%s\" was created successfully.\n", 
                   client.c_str(), group.c_str());
        } else {
            printf("%s: ERROR: failed to create group \"%s\"\n", 
                   client.c_str(), group.c_str());
        }
    }
    else {
        if(success) {
            printf("Group \"%s\" was created successfully.\n", group.c_str());
        } else {
            printf("ERROR: failed to create group \"%s\".\n", group.c_str());
        }
    }
}

/*
 * Description: Prints to the screen the messages of "send" command
 * server: true for server, false for client
 * success: Whether the operation was successful
 * sender: true for sender client, false for receiver client.
 * client: Client name
 * name: Name of the client/group destination of the message
 * message: The message
*/
void print_send(bool server, bool sender, bool success, const std::string& client,
                const std::string& name, const std::string& message) {
    if(server) {
        if(success) {
            printf("%s: \"%s\" was sent successfully to %s.\n", 
                   client.c_str(), message.c_str(), name.c_str());
        } else {
            printf("%s: ERROR: failed to send \"%s\" to %s.\n", 
                   client.c_str(), message.c_str(), name.c_str());
        }
    }
    else if (sender) {
	    if (success) {
		    printf("Sent successfully.\n");
	    } else {
		    printf("ERROR: failed to send.\n");
	    }
    }
	else {
		printf("%s: %s\n", client.c_str(), message.c_str());
    }
}

/*
 * Description: Prints to the screen the messages recieved by the client
 * client: Name of the sender
 * message: The message
*/
void print_message(const std::string& client, const std::string& message) {
    printf("%s: %s\n", client.c_str(), message.c_str());
}

/*
 * Description: Prints to the screen the messages of "who" command in the server
 * client: Name of the sender
*/
void print_who_server(const std::string& client) {
    printf("%s: Requests the currently connected client names.\n", client.c_str());
}

/*
 * Description: Prints to the screen the messages of "who" command in the client
 * connectedClients: a string containing the names of all clients seperated by comma.
*/
void print_who_client(const std::string& connectedClients) {
	printf("%s\n", connectedClients.c_str());
}

/*
 * Description: Prints to the screen the messages of "exit" command
 * server: true for server, false for client
 * client: Client name
*/
void print_exit(bool server, const std::string& client) {
    if(server) {
        printf("%s: Unregistered successfully.\n", client.c_str());
    } else {
        printf("Unregistered successfully.\n");
    }
}

/*
 * Description: Prints to the screen the messages of invalid command
*/
void print_invalid_input() {
    printf("ERROR: Invalid input.\n");
}

/*
 * Description: Prints to the screen the messages of system-call error
*/
void print_error(const std::string& function_name, int error_number) {
    printf("ERROR: %s %d.\n", function_name.c_str(), error_number);
}

/*
 * Description: Reads a message from the file associated with the given file-descriptor (fd).
 *              Makes sure that the data is being read entirely.
 * fd: the file descriptor of which we should read.
 * Returns the message that was read from file associated with the given file-descriptor (fd).
*/
std::string readData(int fd) {
	int bytesAlreadyRead = 0;
	int bytesReadThisPass = 0;
	auto bufferP = (char*) calloc(WA_MAX_INPUT, sizeof(char));
	char* buf = bufferP;
	if (!((bool) bufferP)) {
		return READ_FAILURE;
	}

	// First, we parse the number of bytes we need to read from the message.
	// This number of bytes is encoded in the first 4 (BYTES_TO_READ_LENGTH) chars
	// of the buffer we read from.
	if( read(fd, buf, BYTES_TO_READ_LENGTH) <= 0) {
		free(bufferP);
		return READ_FAILURE;
	}
	auto bytesToRead = (int) strtol(buf, nullptr, DECIMAL_BASE) +
	                   BYTES_TO_READ_LENGTH;

	bytesAlreadyRead += BYTES_TO_READ_LENGTH;
	buf += BYTES_TO_READ_LENGTH;

	// Now, we read the message itself, while making sure the message is being read entirely.
	while (bytesAlreadyRead < bytesToRead) {
		bytesReadThisPass = (int) read(fd, buf, (unsigned int) (bytesToRead - bytesAlreadyRead));
		if (bytesReadThisPass <= 0) {
			free(bufferP);
			return READ_FAILURE;
		}
		bytesAlreadyRead += bytesReadThisPass;
		buf += bytesReadThisPass;
	}
	std::string message(bufferP + BYTES_TO_READ_LENGTH);
    // message now points to the beginning of the message itself.
	free(bufferP);
	return message;
}

/*
 * Description: Writes a message whose length is the given number of bytes into
 *              the file associated with the given file-descriptor (fd).
 *              Makes sure that the data is written entirely.
 * fd: the file descriptor into which we should write.
 * message: the message we need to write.
*/
int writeData(int fd, std::string& message) {
	int bytesAlreadyWritten = 0;
	int bytesWrittenThisPass = 0;
	auto bytesToWrite = (int) message.length();
	std::string bytesToWriteString;

	// First we wrap the length of the message with zeros. Thus, its length is exactly 4 chars.
	if (bytesToWrite < 10) {
		bytesToWriteString = "000" + std::to_string(bytesToWrite);
	}
	else if (bytesToWrite < 100) {
		bytesToWriteString = "00" + std::to_string(bytesToWrite);
	}
	else if (bytesToWrite < 1000) {
		bytesToWriteString = "0" + std::to_string(bytesToWrite);
	} else {
		bytesToWriteString = std::to_string(bytesToWrite);
	}

	// We encode the length of the message in the first 4 chars of the message.
	std::string newMessage = bytesToWriteString + message;
	auto messageBuffer = (char*) newMessage.c_str();

	bytesToWrite += BYTES_TO_READ_LENGTH;

	while (bytesAlreadyWritten < bytesToWrite) {
		bytesWrittenThisPass = (int) write(fd, messageBuffer,
		                                  (unsigned int) (bytesToWrite - bytesAlreadyWritten));
		if (bytesWrittenThisPass > 0) {
			bytesAlreadyWritten += bytesWrittenThisPass;
			messageBuffer += bytesWrittenThisPass;
		}
		else {
			return WRITE_FAILURE;
		}
	}
	return bytesAlreadyWritten;
}

/*
 * Description: Parse user input from the argument "command". The other arguments
 * are used as output of this function.
 * command: The user input
 * commandT: The command type
 * name: Name of the client/group
 * message: The message
 * clients: a vector containing the names of all clients
*/
void parse_command(const std::string& command, command_type& commandT, 
                   std::string& name, std::string& message, 
                   std::vector<std::string>& clients) {
    char c[WA_MAX_INPUT];
    const char *s; 
    char *saveptr;
    name.clear();
    message.clear();
    clients.clear();
    
    strcpy(c, command.c_str());
    s = strtok_r(c, " ", &saveptr);
    
    if(!strcmp(s, "create_group")) {
        commandT = CREATE_GROUP;
        s = strtok_r(NULL, " ", &saveptr);

	    if(!s) {
            commandT = INVALID;
            return;
        } else {
            name = s;
            while((s = strtok_r(NULL, ",", &saveptr)) != NULL) {
                clients.emplace_back(s);
            }
        }
    } else if(!strcmp(s, "send")) {
        commandT = SEND;
        s = strtok_r(NULL, " ", &saveptr);

	    if(!s) {
            commandT = INVALID;
            return;
        } else {
            name = s;
            s = strtok_r(NULL, " ", &saveptr);
		    if(!s) {
			    commandT = INVALID;
			    return;
		    }
		    message = command.substr(name.size() + 6); // 6 = 2 spaces + "send"
        }
    } else if(!strcmp(s, "who")) {
        commandT = WHO;
    } else if(!strcmp(s, "exit")) {
        commandT = EXIT;
    } else {
        commandT = INVALID;
    }
}
