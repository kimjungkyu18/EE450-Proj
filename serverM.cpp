#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>

#define LOCALHOST "127.0.0.1"
#define SERVM_TCP_PORT 25745
#define SERVM_UDP_PORT 24745
#define SERVC_UDP_PORT 21745
#define SERVEE_UDP_PORT 23745
#define SERVCS_UDP_PORT 22745
#define BACKLOG 10 
#define MAXLINE 1024

using namespace std;

// Global Variables
struct sockaddr_in servaddr, credaddr, servC_addr, servEE_addr, servCS_addr;
int servsock, udpsock;

void die(const char* msg)
{
	perror(msg);
	exit(1);
}

void sigint_handler(int sig)
{
	printf("Main server closed manually.\n");
	close(servsock);
	close(udpsock);
	exit(1);
}

void init_TCP_servsock()
{
	// Create servsock
	if ((servsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket failed.");
		exit(1);
	}
	
	// Create server address structure
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(LOCALHOST);
	servaddr.sin_port = htons(SERVM_TCP_PORT);

	// Bind servsock to server address structure
	if (bind(servsock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		die("Bind failed");
	}

	// Start listening for incoming connections
	 if (listen(servsock, BACKLOG) < 0) {
	 	die("listen failed");
	 }
}

void init_UDP_udpsock()
{
	// Create UDP udpsock
	if ((udpsock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		die("udpsock failed");
	}

	// Create server address structure of udp sock at main server side
	memset(&credaddr, 0, sizeof(credaddr));
	credaddr.sin_family = AF_INET;
	credaddr.sin_addr.s_addr = inet_addr(LOCALHOST);
	credaddr.sin_port = htons(SERVM_UDP_PORT);

	// Bind UDP socket to explicit port
	if ( bind(udpsock, (struct sockaddr *) &credaddr, sizeof(credaddr)) < 0) {
		die("udpsock bind failed");
	}

	// Create server address structure of cred server at cred side
	memset(&servC_addr, 0, sizeof(servC_addr));
	servC_addr.sin_family = AF_INET;
	servC_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
	servC_addr.sin_port = htons(SERVC_UDP_PORT);

	// Create server address structure of EE server at EE side
	memset(&servEE_addr, 0, sizeof(servEE_addr));
	servEE_addr.sin_family = AF_INET;
	servEE_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
	servEE_addr.sin_port = htons(SERVEE_UDP_PORT);

	// Create server address structure of CS server at CS side
	memset(&servCS_addr, 0, sizeof(servCS_addr));
	servCS_addr.sin_family = AF_INET;
	servCS_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
	servCS_addr.sin_port = htons(SERVCS_UDP_PORT);


}

void encrypt(char word[])
{
	for (unsigned int i = 0; i < strlen(word); i++) {
		if (word[i] >= 97 && word[i] <= 122) {   // If lowercase letter
			if (word[i] <= 118) { // Not a wraparound case
				word[i] += 4;
			}
			else {  // Wraparound
				word[i] = 97 + 3 - 122 + word[i];
			}
		}
		else if (word[i] >= 65 && word[i] <= 90) {   // If uppercase letter
			if (word[i] <= 86) {
				word[i] += 4;
			}
			else {  // Wraparound
				word[i] = 65 + 3 - 90 + word[i];
			}
		}
		else if (word[i] >= 48 && word[i] <= 57) {  // If number
			if (word[i] <= 53) {  // Not a wraparound
				word[i] += 4;
			}
			else {  // Wraparound
				word[i] = 48 + 3 - 57 + word[i];
			}
		}
	}
	printf("encrypted word: %s\n", word);
}

int main(void)
{
	// Setup sighandler
	// Source: https://beej.us/guide/bgipc/html/multi/signals.html
	struct sigaction sa;
	void sigint_handler(int sig);
	sa.sa_handler = sigint_handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGINT, &sa, NULL) == -1) {
		die("sigaction failed\n");
	}
	// Adapted from: http://www.cs.columbia.edu/~jae/3157/
	// Specifically, tcp-sender.c and tcp-recver.c
	init_TCP_servsock();
	init_UDP_udpsock();
	printf("The main server is up and running.\n");

	int r;
	char buf[MAXLINE];
	int clntsock;
	socklen_t clntlen;
	struct sockaddr_in clntaddr;

	// Accept incoming connections
	while(1) {

		// Accepted a client
		clntlen = sizeof(clntaddr);
		if ((clntsock = accept(servsock, (struct sockaddr *) &clntaddr, &clntlen)) < 0) {
			perror("Accept failed");
			exit(1);
		}
		
		// Loop a maximum of three times for a client to be authenticated
		int numTries = 0;
		bool authenticated = false;
		char* unencrypted_username;
		while (numTries < 3){
			// Receive credentials line
			r = recv(clntsock, buf, sizeof(buf), 0);
			buf[r] = '\0';
			if (r < 0) {
				fprintf(stderr, "ERR: recv failed\n");
			}

			// Split the string for username and password
			char* unencrypted_password;
			char credentials_buf[102];
			char* token;
			strcpy(credentials_buf, buf);
			token = strtok(credentials_buf, ",");       // Get the username
			unencrypted_username = token;
			token = strtok(NULL, ",");
			unencrypted_password = token;
			printf("The main server received the authentication for %s using TCP over port %d\n", unencrypted_username, SERVM_TCP_PORT);

			// Encrypt the username and password
			encrypt(unencrypted_username);
			encrypt(unencrypted_password);
			char credentials[102];
			strcpy(credentials, unencrypted_username);
			char ch = ',';
			strncat(credentials, &ch, 1);
			strncat(credentials, unencrypted_password, strlen(unencrypted_password));

			// Send encrypted credentials to server
			if ( sendto(udpsock, credentials, sizeof(credentials), 0, (const struct sockaddr *) &servC_addr, sizeof(servC_addr)) < 0) {
				fprintf(stderr, "ERR: sendto failed\n");
			}
			printf("The main server sent an authentication request to serverC.\n");
				

			// Receive authentication results from credserv
			int n;
			socklen_t len;
			n = recvfrom(udpsock, buf, sizeof(buf), 0, (struct sockaddr *) &servC_addr, &len);
			if (n < 0) {
				fprintf(stderr, "ERR: recvfrom failed\n");
			}
			buf[n] = '\0';
			printf("The main server received the result of the authentication request from ServerC using UDP over port %d.\n", SERVM_UDP_PORT);


			// Increment numTries or break loop on success
			if (strcmp(buf, "MATCH") == 0) {
				// We have succesfully authenticated user. Send results to client 
				authenticated = true;
				if (send(clntsock, buf, n, 0) < 0) {
					fprintf(stderr, "ERR: send failed \n");
				}
				printf("The main server sent the authentication result to the client.\n");
				break;
			}
			else {  // Login unsucessfull
				// Send authentication result to client
				if (send(clntsock, buf, n, 0) < 0) {
					fprintf(stderr, "ERR: send failed \n");
				}
				numTries = numTries + 1;
			}
		}
		cout << "Debug: unencrypted_username checkpoint=" << unencrypted_username << endl;

		// We have finished the authentication process.
		// Begin query process.
		char query[MAXLINE];
		char user[strlen(unencrypted_username)+1];
		strcpy(user, unencrypted_username);
		user[strlen(unencrypted_username)] = '\0';
		int rq;
		if (authenticated) {
			// Wait for queries as long as client socket stays open
			while(1) {
				if ( (rq = recv(clntsock, query, sizeof(query), 0)) == 0 ){  // if recv returns zero, connection has been closed
					break;
				}
				query[rq] = '\0';
				if (rq < 0) { fprintf(stderr, "ERR: query recv failed"); }

				/*
				 *  Check if its multiple courses or not
				 *  Source: https://www.geeksforgeeks.org/program-to-parse-a-comma-separated-string-in-c/
				 */
				string querystr = query;
				vector<string> courses;
				vector<string> tokens2;
				stringstream ss(querystr);
				stringstream ss2(querystr);

				while(ss.good()){
					string substr;
					getline(ss, substr, ' ');
					courses.push_back(substr);
				}

				// MULTIPLE COURSE CASE
				if (courses.size() > 1){
					cout << "MULTIPLE COURSE CASE REACHED!!" << endl;
					vector<string> results;
					// DEBUGGING
					for (string course : courses){
						cout << "DEBUGGING SERVERM RESULTS: " << course << endl;
					}

					// Send a request to corresponding department servers
					for (string course : courses){
						char course_query[MAXLINE];
						strcpy(course_query, course.c_str());
						char course_query_buf[MAXLINE];
						string course_query_res;
						printf("Debugging sendtoEE: %s\n", course_query);

						// Sendto EE serv
						if (strncmp(course_query, "EE", 2) == 0) {
							if ( sendto(udpsock, course_query, sizeof(course_query), 0, (struct sockaddr *) &servEE_addr, sizeof(servEE_addr)) < 0 ){
								fprintf(stderr, "ERR: sendto query failed\n");
							}
							printf("The main server sent a request to serverEE.\n");

							// Recvfrom EE serv
							socklen_t len;
							if ( (rq = recvfrom(udpsock, course_query_buf, sizeof(course_query_buf), 0, (struct sockaddr *) &servEE_addr, &len))  < 0){
								fprintf(stderr, "ERR: recvfrom query failed\n");
							}
							printf("The main server received the response from severEE using UDP over port %d.\n", SERVEE_UDP_PORT);
							course_query_res = course_query_buf;
							results.push_back(course_query_res);
							cout << "Debuggin recvfromEE: " << course_query_res << endl;
						}

						// Send to CS server
						else if (strncmp(course_query, "CS", 2) == 0) { 
							if ( sendto(udpsock, course_query, sizeof(course_query), 0, (struct sockaddr *) &servCS_addr, sizeof(servCS_addr)) < 0 ){
								fprintf(stderr, "ERR: sendto query failed\n");
							}
							printf("The main server sent a request to serverCS.");

							// Recvfrom CS serv
							socklen_t len;
							if ( (rq = recvfrom(udpsock, course_query_buf, sizeof(course_query_buf), 0, (struct sockaddr *) &servCS_addr, &len))  < 0){
								fprintf(stderr, "ERR: recvfrom query failed\n");
							}
							printf("The main server received the response from severCS using UDP over port %d.\n", SERVCS_UDP_PORT);
							course_query_res = course_query_buf;
							results.push_back(course_query_res);
						}

					}
					// Aggregate query results to send to client
					string aggregated_res_str = "";
					char aggregated_res_buf[MAXLINE];
					for (string res : results){
						aggregated_res_str.append(res);
					}
					strcpy(aggregated_res_buf, aggregated_res_str.c_str());
					printf("Debugging aggregated result: %s\n", aggregated_res_buf);


					// Send query results to client
					if (send(clntsock, aggregated_res_buf, aggregated_res_str.length(), 0) < 0) {
						fprintf(stderr, "ERR: query send failed\n");
					}
					printf("The main server sent the query information to the client.\n");
				}
				else { // NON MULTIPLE COURSE CASE
					// Parse the query
					while(ss2.good()){
						string substr;
						getline(ss2, substr, ',');
						tokens2.push_back(substr);
					}

					string query_str = "";
					for (string str : tokens2){
						cout << "Debugging: tokens2.str=" << str << endl;
						query_str.append(str).append(",");
					}
					query_str.pop_back(); // Remove last comma
					cout << "Debugging: query_str=" << query_str << endl;

					

					// Parse query
					char code[MAXLINE];
					char category[MAXLINE];
					char query_buf[MAXLINE];

					strcpy(code, tokens2.front().c_str());
					strcpy(category, tokens2.back().c_str());
					strcpy(query_buf, query_str.c_str());
					printf("Debug: code=%s, category=%s, query_buf=%s, unencrypted_username=%s\n", code, category, query_buf, user);

					printf("The main server received from %s to query course %s about %s using TCP over port %d.\n", user, code, category, SERVM_TCP_PORT);

					/*
					 *  Send query to respective server
					 */
					char query_res[MAXLINE];
					
					// Sendto EE serv
					if (strncmp(code, "EE", 2) == 0) {
						if ( sendto(udpsock, query, sizeof(query), 0, (struct sockaddr *) &servEE_addr, sizeof(servEE_addr)) < 0 ){
							fprintf(stderr, "ERR: sendto query failed\n");
						}
						printf("The main server sent a request to serverEE.");

						// Recvfrom EE serv
						socklen_t len;
						if ( (rq = recvfrom(udpsock, query_res, sizeof(query_res), 0, (struct sockaddr *) &servEE_addr, &len))  < 0){
							fprintf(stderr, "ERR: recvfrom query failed\n");
						}
						printf("The main server received the response from severEE using UDP over port %d.\n", SERVEE_UDP_PORT);

						// Send query results to client
						if (send(clntsock, query_res, rq, 0) < 0) {
							fprintf(stderr, "ERR: query send failed\n");
						}
						printf("The main server sent the query information to the client.\n");
					}

					// Send to CS server
					else if (strncmp(code, "CS", 2) == 0) { 
						if ( sendto(udpsock, query, sizeof(query), 0, (struct sockaddr *) &servCS_addr, sizeof(servCS_addr)) < 0 ){
							fprintf(stderr, "ERR: sendto query failed\n");
						}
						printf("The main server sent a request to serverCS.");

						// Recvfrom CS serv
						socklen_t len;
						if ( (rq = recvfrom(udpsock, query_res, sizeof(query_res), 0, (struct sockaddr *) &servCS_addr, &len))  < 0){
							fprintf(stderr, "ERR: recvfrom query failed\n");
						}
						printf("The main server received the response from severCS using UDP over port %d.\n", SERVCS_UDP_PORT);

						// Send query results to client
						if (send(clntsock, query_res, rq, 0) < 0) {
							fprintf(stderr, "ERR: query send failed\n");
						}
						printf("The main server sent the query information to the client.\n");
					}

					// Immediately send no course found to client
					else {  // Non-ee or cs course
						sprintf(query_res, "%s,CRS_NOT_FOUND", code);
						if (send(clntsock, query_res, strlen(query_res) + 1, 0) < 0) {
							fprintf(stderr, "ERR: query send failed\n");
						}
						printf("The main server sent the query information to the client.\n");
					}
				}
			}
		}
		// At this point, all three authentication tries were used up
		// Go back to accepting new TCP connections 
	}

}
