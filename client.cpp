#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>

#define LOCALHOST "127.0.0.1"
#define SERV_TCP_PORT 25745
#define MAXLINE 1024

using namespace std;

int sock;
struct sockaddr_in servaddr;

void die(const char* msg) {
	perror(msg);
	exit(1);
}

void sigint_handler(int sig)
{
	printf("Client closed manually.\n");
	close(sock);
	exit(1);
}

void create_TCP_socket()
{
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Socket failed");
	}

	// Create server address structure
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(LOCALHOST);
	servaddr.sin_port = htons(SERV_TCP_PORT);
}

void connect_TCP_socket()
{
	if (connect(sock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) { 
		perror("Connect failed");
		exit(1);
	}
}

int main()
{
	/**
	 *  STEP 0: Setup sighandler
	 *  Source: https://beej.us/guide/bgipc/html/multi/signals.html
	 */
	struct sigaction sa;
	void sigint_handler(int sig);
	sa.sa_handler = sigint_handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
		die("sigaction failed");
    }

	/**
	 * STEP 1: Create TCP socket and connect to serverM TCP socket.
	 * Adapted from: http://www.cs.columbia.edu/~jae/3157/
	 * Specifically, tcp-sender.c
	 */
	create_TCP_socket();
	connect_TCP_socket();

	/*
	 * STEP 2: Loop for authencation requests up to 3 times..
	 */	
	printf("The client is up and running.\n");

	int numTries = 0;
	char username[52];
	char password[52];
	int local_port;
	while (1) {
		if (numTries >= 3) {
			printf("Authentication failed for 3 attempts. Client will shut down.\n");
			close(sock);
			exit(1);
		}
		
		// Prompt for credentials
		printf("Please enter the username: ");
		fgets(username, sizeof(username), stdin);    // Get the credentials
		username[strcspn(username, "\n")] = '\0';    // Delete newline
		printf("Please enter the password: ");
		fgets(password, sizeof(password), stdin);    // Get the credentials
		password[strcspn(password, "\n")] = '\0';    // Delete newline

		// Format credentials onto one line to send
		char credentials[102] = "";
		strcat(credentials, username);
		strcat(credentials, ",");
		strcat(credentials, password);


		/*
		 * STEP 3: Send authentication request to serverM.
		 */
		size_t len = strlen(credentials);
		if (send(sock, credentials, len, 0) < 0) {
			die("send failed\n");
		}
		printf("%s sent an authentication request to the main server.\n", username);

		/*
		 * STEP 4: Receive authentication result from main server.
		 */
		int r;
		char buf[MAXLINE];
		r = recv(sock, buf, sizeof(buf), 0);
		if (r < 0) {
			die("recv failed\n");
		}

		// Dynamically find client TCP port
		// Source: https://stackoverflow.com/questions/2360304/get-the-client-port-in-c-after-a-call-to-connect
		// Source: getsockname() man page
		struct sockaddr_in local_address;
		socklen_t addr_size = sizeof(local_address);
		getsockname(sock, (sockaddr*)&local_address, &addr_size);
		local_port = ntohs(local_address.sin_port);

		// Compare the results
		if (strcmp(buf, "MATCH") == 0) {
			printf("%s received the result of authentication using TCP over port %d. Authentication is successful\n", username, local_port);
			break;
		} else if (strcmp(buf, "WRONG_PASS") == 0) {
			printf("%s received the result of authentication using TCP over port %d. Authentication failed: Password does not match\nAttempts remaining: %d\n", username, local_port, 2 - numTries);
		} else {
			printf("%s received the result of authentication using TCP over port %d. Authentication failed: Username Does not exist\nAttempts remaining: %d\n", username, local_port, 2 - numTries);
		}
		numTries = numTries + 1;
	}

	/*
	 * STEP 5: Loop for queries.
	 * By this point, we have ben succesfully authorized.
	 * If all tries have been used up, if statement will catch that in the above loop
	 * And this while look will never run
	 */
query:
	while(1) {
		// Prompt for course code to query
		char coursecode[MAXLINE];
		char category[MAXLINE];
		char queryline[MAXLINE];
		printf("Please enter the course code to query: ");
		fgets(queryline, sizeof(queryline), stdin);
		queryline[strcspn(queryline, "\n")] = '\0';    // Delete newline

		// Parse the queryline for multiple courses
		// Source: https://stackoverflow.com/questions/236129/how-do-i-iterate-over-the-words-of-a-string
		string line = queryline;
		istringstream iss(line);
		vector<string> courses{istream_iterator<string>{iss}, istream_iterator<string>{}};

		// MULTIPLE COURSE CASE
		if (courses.size() > 1){
			// Check for max 10 possible queries
			if (courses.size() >= 10){
				printf("The maximum number of courses allowed in a single request is 9.\n");
				printf("\n\n-----Start a new request-----\n");
				goto query;
			}
			
			/*
			 *  Format the query line to send to main server
			 */
			string multp_course_line = "";
			for (string course: courses){
				multp_course_line.append(course).append(" ");
			}
			multp_course_line.pop_back();  // Get rid of extra space at end 
			char multp_course_buf[MAXLINE];
			strcpy(multp_course_buf, multp_course_line.c_str());

			/*
			 *  Send the formatted query to main server
			 */
			size_t len = strlen(multp_course_buf) + 1;
			if (send(sock, multp_course_buf, len, 0) < 0) {
				die("Query send failed\n");
			}
			printf("%s sent a request with multiple CourseCode to the main server.\n", username);

			// Receive query results from main server
			int rq;
			char query_res[MAXLINE];
			rq = recv(sock, query_res, sizeof(query_res), 0);
			printf("The client received the response from the Main server using TCP over port %d.\n", local_port);
			if (rq < 0) { die("Query recv failed\n"); }

			// Print results to client console
			printf("%s", query_res);
		}

		else{  // Single course case
			// Prompt for category 
			printf("Please enter the category (Credit / Professor / Days / CourseName): ");
			fgets(category, sizeof(category), stdin);
			category[strcspn(category, "\n")] = '\0';    // Delete newline
			
			// Format query before sending to main server
			char query[MAXLINE];
			strcpy(coursecode, courses.front().c_str());
			strcpy(query, coursecode);
			strcat(query, ",");
			strcat(query, category);

			// Send formatted query to main server
			size_t len = strlen(query) + 1;
			if (send(sock, query, len, 0) < 0) {
				die("Query send failed\n");
			}
			printf("%s sent a request to the main server.\n", username);
	
			// Receive query results from main server
			int rq;
			char query_res[MAXLINE];
			rq = recv(sock, query_res, sizeof(query_res), 0);
			printf("The client received the response from the Main server using TCP over port %d.\n", local_port);
			if (rq < 0) { die("Query recv failed\n"); }
	
	        // Split the query result for username and password
			// Adapted from: https://www.geeksforgeeks.org/how-to-split-a-string-in-cc-python-and-java/
	        char* category_res;
	        char* token;
	        token = strtok(query_res, ",");       // Get the username
	        token = strtok(NULL, ",");
	        category_res = token;
	
			// Display query results to client
			if (strcmp(category_res, "CATEGORY_NOT_FOUND") == 0 || strcmp(category_res, "CRS_NOT_FOUND") == 0){
				printf("Didn't find the course: %s.\n", coursecode);
			}
			else {
				printf("The %s of %s is %s.\n", category, coursecode, category_res);
			}
		}
		printf("\n\n-----Start a new request-----\n");
	}
	close(sock);
	return(0);
}
