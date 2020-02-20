# Whatsapp-Like
An implementation of a Whatsapp-like service using TCP/IP protocol.
The system is composed of one server and multiple clients.
After a client connects to the service, it can send and recieve messages from other clients connected to the server.


The command line for running the server is:

```
whatsappServer <port_number>
```
e.g.
```
whatsappServer 8875
```


The command line for running the client is:
```
whatsappClient <client_name> <server_address> <server_port_number> 
```
e.g.
```
whatsappClient Daniel 127.0.0.1 8875
```
                                                
In order to use the Whatsapp service, the client can use the following commands:

    1.  create_group <group_name> <list_of_client_names>
        (e.g. create_group family Dad,Mom,Avi,Yael)
        Description: Sends the server a request to create a new group named <group_name> with <list_of_client_names> as group members.
                     The group name should be unique (i.e. no other group or client with this name is allowed) and includes only letters and digits.
                     
    2.  send <client_or_group_name> <message>
        (e.g. send Avi Hey man, what's up?)
        Description: If <client_or_group_name> is a client name, it sends:
                     "<sender_client_name>: <message>" to the specified client
                     (e.g. if client Yael used the command: "send Mom Thanks Mom!", client Mom should recieve: "Yael: Thanks Mom!").
                     
                     If <client_or_group_name> is a group name, it sends:
                     "<sender_client_name>: <message>" to all group members (except the sender client).
                     (e.g. if client Yael used the command: "send family Hi all!", clients Dad, Mom & Avi should recieve: "Yael: Hi all!").

    3.  who
        Description: Sends a request to the server to recieve a list of currently connected client names (alphabetically order).

    4.  exit
        Description: Unregisters the client from the server and removes it from all groups.


## Files
whatsappio.h -- header file for whatsapp.cpp

whatsapp.cpp -- handles I/O of the server & the client

whatsappServer.cpp -- implementation of the server side of communication protocol

whatsappClient.cpp -- implementation of the client side of communication protocol

Makefile -- a Makefile that compiles the executables

## Remarks
The main challenge in this exercise was to create the 'writeData' & 'readData' functions,
which handles the data transfer between the sever and the client, while making sure the
data is transfered entirely, and without additional "garbage".
What made these functions quite hard to code was adding the length of the transfered message
to the beginning of the written message, and parsing it correctly with the 'readData' function.

Other than that, it took quite a lot of time to make sure that every invalid input - whether it
should be handeled in the client side or in the server side - is taken care correctly, and the
relevant Error-message is indeed printed.
