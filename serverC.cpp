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
#include <list>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#define LOCALHOST "127.0.0.1"
#define CREDSERV_UDP_PORT 21745
#define MAINSERV_UDP_PORT 24745
#define MAXLINE 1024

using namespace std;

// Credential struct definition
struct credential{
	string username;
	string password;
};

// Global Variables
int sock;
struct sockaddr_in sockaddr, serverM_addr;
list<credential> cred_list;

void die(const char* msg){ perror(msg); exit(1); }

void init_UDP_sock()
{
	// Create socket
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		die("serverC udp socket failed");
	}

	// Create address structure
	memset(&sockaddr, 0, sizeof(sockaddr));
	memset(&serverM_addr, 0, sizeof(serverM_addr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = inet_addr(LOCALHOST);
	sockaddr.sin_port = htons(CREDSERV_UDP_PORT);

	// Bind socket to explicit port
	if ( bind(sock, (const struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
		die("serverC bind failed");
	}
}

void parseCredentials()
{
	// Open the file in an ifstream
	// Source: https://cplusplus.com/doc/tutorial/files/
	// Source: ifstream man page
	ifstream ifs;
	ifs.open("cred.txt");
	if (!ifs.is_open()) {
		die("stream open failed");
	}
	
	// Parse the file and store credentials
	string username, password, line;
	while (getline(ifs, line)) {
		istringstream line_stream(line);
		getline(line_stream, username, ',');
		getline(line_stream, password, ',');

		// Store it in the credential list
		struct credential cred = {username, password};
		cred_list.push_back(cred);
		// cout << cred.username << "," << cred.password << endl;
	} 
}

string validateCredentials(string line)
{
	// Parse the line
	string username, password;
	istringstream line_stream(line);
	getline(line_stream, username, ',');
	getline(line_stream, password, ',');

	string res = "NO_USER_MATCH";
	for (struct credential cred : cred_list) {
		if (cred.username == username && cred.password == password) {
			res = "MATCH";
			return res;
		} else if (cred.username == username && cred.password != password) {
			res = "WRONG_PASS";
			return res;
		} 
	}

	return res;
}

int main()
{
	init_UDP_sock();
	parseCredentials();
	printf("The ServerC is up and running using UDP on port %d\n", CREDSERV_UDP_PORT);
	
	// Infinitely loop to receive authentication requests
	while(1) {
		char buffer[MAXLINE];
		socklen_t len = sizeof(serverM_addr);
		int n;
		int s;

		n = recvfrom(sock, (char *)buffer, MAXLINE, 0, ( struct sockaddr *) &serverM_addr, &len);
		if (n < 0) {fprintf(stderr, "ERR: recvfr failed\n"); }
		printf("The ServerC received an authentication request from the Main Server.\n");
		
		// Format the result
		string res = validateCredentials(buffer);
		char result[res.length()+1];
		strcpy(result, res.c_str());

		// Send authentication result to serverM	
		s = sendto(sock, (char *)result, sizeof(result), 0, (struct sockaddr *) &serverM_addr, len);
		if (s < 0) {fprintf(stderr, "ERR: sendto failed\n");}
		printf("The ServerC finished sending the response to the Main Server.\n");
	}
}
