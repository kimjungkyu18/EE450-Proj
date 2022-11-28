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
#include <algorithm>

#define LOCALHOST "127.0.0.1"
#define EESERV_UDP_PORT 23745
#define MAXLINE 1024

using namespace std;

// Function prototypes
string queryCourses(string line);

// Course struct definition
struct course{
	string code;
	string credits;
	string professor;
	string days;
	string name;
};

// Global Variables
int servsock;
struct sockaddr_in servaddr, cliaddr;
list<course> course_list;
string code, category;

void die(const char* msg){ perror(msg); exit(1); }

void init_UDP_sock()
{
	// Create socket
	if ((servsock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		die("serverEE socket failed");
	}

	// Create address structure
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(LOCALHOST);
	servaddr.sin_port = htons(EESERV_UDP_PORT);

	// Bind socket to explicit port
	if ( bind(servsock, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		die("serverEE bind failed");
	}
}

void parseCourses()
{
	// Open the file in an ifstream
	ifstream ifs;
	ifs.open("ee.txt");
	if (!ifs.is_open()) {
		die("stream open failed");
	}
	
	// Parse the file and store credentials
	string code, credits, professor, days, name, line;
	while (getline(ifs, line)) {
		istringstream line_stream(line);
		getline(line_stream, code, ',');
		getline(line_stream, credits, ',');
		getline(line_stream, professor, ',');
		getline(line_stream, days, ',');
		getline(line_stream, name, ',');

		// Store it in the credential list
		struct course crs = {code, credits, professor, days, name};
		course_list.push_back(crs);
	} 
}

string parseLine(string line)
{
	vector<string> tokens;
	stringstream ss(line);

	while(ss.good()){
		string substr;
		getline(ss, substr, ',');
		tokens.push_back(substr);
	}

	string res = "";
	if (tokens.size() == 1){  // This from a multiple course request, send ALL info
		printf("The ServerEE received a full-info course request from the Main Server.\n");
		
		for (struct course crs: course_list) {
			if (tokens.front() == crs.code) {
				cout << "The course information for " << crs.code << " has been found. Credit: " << crs.credits << ". Professor: " << crs.professor << ". Days: " << crs.days << ". Course Name: " << crs.name << ".\n";
				res.append(crs.code).append(": ").append(crs.credits).append(", ").append(crs.professor).append(", ").append(crs.days).append(", ").append(crs.name).append("\n");
				return res;
			}
		}
		// No course match found
		res.append("Didn't find the course: ").append(tokens.front()).append("\n");
		cout << "Didn't find the course: " << tokens.front() << "." << endl;
		return res;
	}
	else { // This is a single category query
		return queryCourses(line);
	}
}

string queryCourses(string line)
{
	// Parse the line
	istringstream line_stream(line);
	getline(line_stream, code, ',');
	getline(line_stream, category, ',');
	cout << "The ServerEE received a request from the Main Server about the " << category << " of " << code << ".\n";
	
	// Query the db
	string res = "";
	bool found = false;
	for (struct course crs : course_list) {
		if (code == crs.code) {  // Found a matching course
			found = true;
			if (category == "Credit") {
				res.append(code).append(",").append(crs.credits);
				cout << "The course information has been found: The " << category << " of " << code << " is " << crs.credits << endl;
			} else if (category == "Professor") {
				res.append(code).append(",").append(crs.professor);
				cout << "The course information has been found: The " << category << " of " << code << " is " << crs.professor << endl;
			} else if (category == "Days") {
				res.append(code).append(",").append(crs.days);
				cout << "The course information has been found: The " << category << " of " << code << " is " << crs.days << endl;
			} else if (category == "CourseName") {
				res.append(code).append(",").append(crs.name);
				cout << "The course information has been found: The " << category << " of " << code << " is " << crs.name << endl;
			} else {
				res.append(code).append(",").append("CATEGORY_NOT_FOUND");
				cout << "The category \"" << category << "\" does not exist." << endl;
			}
		}
	}
	if (!found) {
		res.append(code).append(",").append("CRS_NOT_FOUND");
		cout << "Didn't find the course: " << code << "." << endl;
	}
	return res;
}


int main()
{
	init_UDP_sock();
	parseCourses();
	printf("The ServerEE is up and running using UDP on port %d\n", EESERV_UDP_PORT);
	
	// Infinitely loop to receive db queries
	while(1) {
		char buffer[MAXLINE];
		socklen_t len = sizeof(cliaddr);
		int n;
		int s;

		// Recv from main server
		n = recvfrom(servsock, (char *)buffer, MAXLINE, 0, ( struct sockaddr *) &cliaddr, &len);
		if (n < 0) { fprintf(stderr, "ERR: recvfr failed\n"); }

		// Query db and format results 
		string res = parseLine(buffer);
		char result[res.length()+1];
		strcpy(result, res.c_str());
		result[res.length()] = '\0';

		// Send results of query to main server 
		s = sendto(servsock, result, sizeof(result), 0, (struct sockaddr *) &cliaddr, len);
		if (s < 0) {fprintf(stderr, "ERR: sendto failed\n");}
		cout << "The ServerEE finished sending the response to the Main Server." << endl;
	}
}
