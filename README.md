# Database-Server
common threads is just some useful thread stuff
msg file has format of messages exchanged between server and client
makefile is included

Server:
Multi-threaded server where the parent thread initializes the socket to listening socket, and waits for connection request from a client. Once the server accepts a connection request, it creates a new thread (handler) to handle the client's requests. The server then waits for the next request. The server never terminates. A handler thread terminates only when its client closes the connection. 

Handler Thread:
A handler thread must process each request from a dbclient and send an appropriate response back to the client. 
PUT: This message contains the data record that needs to be stored in the database. On receiving this message, handler appends the record to the database file. (Do not store the entire message. Store only the client data record.) If the write is successful, it will send SUCCESS message to the client. Otherwise the handler sends FAIL message.

2. GET: This request message will contain the id of the record that needs to be fetched. On receiving this message, handler searches the database to find a matching record (record with id field that matches the id in the get message). If a matching record is found, the handler sends SUCCESS message. The message should also contain the record. Otherwise the handler sends FAIL message.


Database Client (dbclient):
First, it sets up a connection with the server. It then prompts the user to choose one of the operations: put, get, quit. Below are the actions taken by dbclient for these operations.

PUT: If the user chooses put, then prompt the user for name and id. Send a put message to dbserver (fill name and id fields of the record), and wait for the response. If the response is SUCCESS, print "put success" message. Otherwise print "put failed" message.

GET: If the user chooses get, then prompt the user for id. Send a get message to server (only fill id field of the record), and wait for the response. If the response is SUCCESS, print name and id. Otherwise print "get failed" message.


QUIT: Close the socket, cleanup, and terminate the program.


Runing the program:
so compile using makefile
then do
./dbserver port
./dbclient hostname port
