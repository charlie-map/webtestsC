#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// all socket related packages
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>

#define PORT "6969" // the port users will connect to
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

// reads all the text in a file into an allocated char *
char *readpage(char *filename, int *length) {
	FILE *file = fopen(filename, "r");

	if (!file) {
		// assumes errorfile exists
		FILE *errorfile = fopen("error.html", "r");

		char *errorreturn = malloc(sizeof(char) * 6558);
		fread(errorreturn, 1, 6558, errorfile);

		return errorreturn; // no file
	}

	int CURR_SIZE = 1024;

	char buffer[CURR_SIZE]; // 1024 characters at a time
	char *returnstring = malloc(sizeof(char) * CURR_SIZE);

	while (fread(buffer, 1, 1024, file)) {
		strcpy(returnstring, buffer);

		CURR_SIZE += 1024;

		// increase return string size
		returnstring = realloc(returnstring, sizeof(char) * CURR_SIZE);
	}

	return returnstring;
}

int main() {

	// connection stuff
	int sock_fd, new_fd; // listen on sock_fd, client on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
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
		if ((sock_fd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
			sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sock_fd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sock_fd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	printf("server go vroom\n");

	while (1) {
		sin_size = sizeof(their_addr);
		new_fd = accept(sock_fd, (struct sockaddr *) &their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
		get_in_addr((struct sockaddr *)&their_addr),
		s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process

			// close(sock_fd); // child doesn't need the listener
			// if (send(new_fd, "Hello, world!", 13, 0) == -1)
			// 	perror("send");

			// make a continuous loop for new_fd while they are still alive
			int *res_length = malloc(sizeof(int)), res_sent;
			char *res = readpage("homepage.html", res_length);

			*res_length = strlen(res);

			// use for making sure the entire page is sent
			while ((res_sent = send(new_fd, res, *res_length, 0)) < *res_length) {
				printf("trying to send %d\n", res_sent);
			}

			free(res_length);
			free(res);

			close(new_fd);
			exit(0);
		}

		close(new_fd);  // parent doesn't need this
	}

	return 0;
}