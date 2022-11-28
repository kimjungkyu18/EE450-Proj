a. Full name
	 Jungkyu Kim

b. Student ID
	 8667430745

c. What I have done:
	I have completed all the mandatory parts as well as the extra credit.

d. Code file descriptions
	serverM authenticates a user and will give course information based on client queries.
	client sends authentication requests to serverM and can send/receive queries once authenticated.
	serverC receives authentication requests from serverM, checks if credentials are correct, and sends an appropriate response to serverM.
	serverEE holds records of all EE courses and will answer any queries received.
	serverCS holds records of all CS courses and will answer any queries received.

e. Format of messages
	Format of messages sent between clients/servers are typically comma delimited and on one line.
	For example a authentication message would be "username,password" and a course query message would be "EE450,Ali Zahid".
	The only exception is when the user queries for multiple courses.
	Then, the messages are space delimited. This allows for multiple courses to be queried in one message.
	Terminal messages strictly follow the project specs except for extra credit messages that are not addressed in the project specs.
	In these cases I use similar messages at my own discretion.

g. Any idiosyncrasy
	I expect my code to work without errors. However, as a mac m1 user I could only test this on AWS with Ubuntu 18.04
	as AWS did not offer 16.04. Based off what TA's have told me, there shouldn't be an issue but I haven't been able to test it.

h. Reused code
	The majority of the socket boilerplate code was adapted from the lecture notes
	of a previous undergraduate I took or from Beej. The source is cited in comments.
	Additionally there are multiple code snippets that are adapted from websites
	like stackoverflow, geeksforgeeks, etc. Those are also cited in the comments.
