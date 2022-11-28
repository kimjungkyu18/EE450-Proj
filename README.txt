a. Full name
	 Jungkyu Kim

b. Student ID
	 8667430745

c. What I have done:
	I have completed all the mandatory parts as well as the extra credit.

d. Code file descriptions
	serverM authenticates a user and will give course information based on client queries.
	client sends authentication requests to serverM and can send queries once authenticated.
	serverC receives authentication requests from serverM and checks the authentication.
	serverEE holds records of all EE courses and will answer any queries received.
	serverCS holds records of all CS courses and will answer any queries received.

e. Format of messages
	Format of messages sent between clients/servers are typically comma delimited and on one line.
	The only exception is when the user queries for multiple courses.
	Then, the messages are space delimited.
	Terminal messages strictly follow the project specs unless not addressed by the specs. 
	In which case I use a similar message at my own discretion.

g. Any idiosyncrasy
	If the client decides to use the extra credit functionality, I have encountered a bug occasionally
	in which the client receives extra bytes, most likely from a buffer that was non
	null terminated. This does not happen if extra credit functionality is not used.

h. Reused code
	The majority of the socket boilerplate code was adapted from the lecture notes
	of a previous undergraduate I took. The source is cited in comments.
	Additionally there are multiple code snippets that are adapted from websites
	like stackoverflow, geeksforgeeks, etc. Those are also cited in the comments.
