#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

// all socket related packages
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>

#define PORT "8080" // the port users will connect to
#define BACKLOG 10    // how many pending connections will queue

void sigchld_handler(int s) {
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa) {

	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *) sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

void *acceptor_function(void *sock_ptr);

// reads all the text in a file into an allocated char *
char *readpage(char *filename, int *length) {
	FILE *file = fopen(filename, "r");

	if (!file) {
		// assumes errorfile exists
		FILE *errorfile = fopen("./views/error.html", "r");

		char *errorreturn = malloc(sizeof(char) * 6544);
		fread(errorreturn, 1, 6544, errorfile);

		return errorreturn; // no file
	}

	size_t *bufferweight = malloc(sizeof(size_t));
	*bufferweight = 0;
	char *getlinestr = malloc(sizeof(char) * 10);
	char *buildstring = "";

	int reallinelen;

	while ((reallinelen = getline(&getlinestr, bufferweight, file)) > 0) {
		
		int new_buildsize = sizeof(char) * (strlen(buildstring) + 1) + sizeof(char) * strlen(getlinestr);
		if (strlen(buildstring) == 0) {
			buildstring = malloc(new_buildsize);
			strcpy(buildstring, getlinestr);
			buildstring[new_buildsize - 1] = '\0';
		} else {
			buildstring = realloc(buildstring, new_buildsize);
			strcpy(buildstring + (sizeof(char) * strlen(buildstring)), getlinestr);
		}

		*length += reallinelen;
		*bufferweight = 0;
	}

	// copy buildstring over with added size
	// second portion for the socket sending
	int page_size = snprintf(NULL, 0, "%d", *length);
	int html_chars = *length;

	printf("test %d\n", html_chars);

	// calculate length of send
	*length = sizeof(char) * html_chars + sizeof(char) * (58 + page_size);
	char *returnstring = malloc(sizeof(char) * *length);

	// copy in the size
	sprintf(returnstring, "HTTP/1.1 200 OK\nContent-Type:text/html\nContent-Length: %d\n\n\n", html_chars);
	// copy in buildstring (moving the starte over by the amount currently in returnstring)
	strcpy(returnstring + (sizeof(char) * strlen(returnstring)), buildstring);

	free(bufferweight);
	free(getlinestr);
	free(buildstring);

	return returnstring;
}

int main() {

	// connection stuff
	int *sock_fd = malloc(sizeof(int)); // listen on sock_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	struct sigaction sa;
	int yes=1;
	int status;

	memset(&hints, 0, sizeof(hints)); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;	  // dont' care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;  // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;	  // fill in my IP for me

	if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}


	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	// find a working socket
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((*sock_fd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(*sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
			sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(*sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
			close(*sock_fd);
			perror("server: bind");
			continue;
		}

		break;
	}


	printf("%d check\n", *sock_fd);
	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(*sock_fd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	printf("server go vroom\n");

	// split acceptor
	pthread_t accept_thread;
	int check = pthread_create(&accept_thread, NULL, &acceptor_function, sock_fd);

	// wait until a user tries to close the server
	while(getchar() != '0');
	pthread_cancel(accept_thread);
	pthread_join(accept_thread, NULL);

	free(sock_fd);

	return 0;
}

void *connection(void *addr_input) {
	int *new_fd = malloc(sizeof(int));
	*new_fd = *(int *) addr_input;

	// make a continuous loop for new_fd while they are still alive
	int *res_length = malloc(sizeof(int)), res_sent;
	*res_length = 0;
	char *res = readpage("./views/homepage.html", res_length);

	// use for making sure the entire page is sent
	while ((res_sent = send(*new_fd, res, *res_length, 0)) < *res_length) {
		//printf("trying to send %d\n", res_sent);
	}

	free(res_length);
	free(res);

	close(*new_fd);
	free(new_fd);
}

void *acceptor_function(void *sock_ptr) {
	int sock_fd = *(int *) sock_ptr;
	int *new_fd = malloc(sizeof(int));
	char s[INET6_ADDRSTRLEN];
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;

	printf("listening");

	while (1) {
		sin_size = sizeof(their_addr);
		*new_fd = accept(sock_fd, (struct sockaddr *) &their_addr, &sin_size);
		if (*new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
		get_in_addr((struct sockaddr *) &their_addr),
		s, sizeof s);
		printf("server: got connection from %s\n", s);

		// at this point we can send the user into their own thread
		pthread_t socket;
		pthread_create(&socket, NULL, &connection, new_fd);
	}

	return 0;
}