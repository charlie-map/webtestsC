#include <stdlib.h>
#include <string.h>

// all socket related packages
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define PORT "6969" // the port users will connect to
#define BACKLOG 10    // how many pending connections will queue

int main() {

	// sample of getaddrinfo listening on host's IP addrees with port 6969
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo; // will point to results

	memset(&hints, 0, sizeof(hints)); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;	  // dont' care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;  // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;	  // fill in my IP for me

	if ((status = getaddrinfo(NULL, MY_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	// servinfo points to a linked list of 1 or more addrinfo structs

	// assuming the first value in servinfo is okay, we can create a socket:
	int socket = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	// then we can bind this socket to the port used in getaddrinfo (can be used for listenin)
	bind(socket, servinfo->ai_addr, servinfo->ai_addrlen);

	// connect!

	connect(socket, servinfo->ai_addr, servinfo->ai_addrlen);

	// listening takes the socket and "backlog" which defines the number of
	// sockets that can wait in queue before being "accept()"ed

	listen(socket, 20);

	// ... do server things ...

	// accept a socket
	int new_sock;
	struct sockaddr_storage their_addr;
	socklen_t addr_size;

	addr_size = sizeof(their_addr);
	new_sock = accept(socket, (struct sockaddr *) &their_addr, &addr_size);

	// ... communicate with socket ...

	char *msg = "charlie-map sent a message!";
	int len, bytes_sent = 0;

	len = strlen(msg);

	// make sure all the bytes send
	while ((bytes_sent = send(new_sock, msg, len, 0)) < len);

	
	// receive the message
	char *receipt;
	int bytes_received = 0;

	while ((bytes_received = recv(new_sock, receipt, len, 0)) < len);


	// disconnect socket
	close(new_sock);

	freeaddrinfo(servinfo); // free the linked list
}