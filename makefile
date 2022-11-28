all: client.cpp serverM.cpp serverC.cpp serverEE.cpp serverCS.cpp
	g++ -g -Wall client.cpp -o client
	g++ -g -Wall serverM.cpp -o serverM
	g++ -g -Wall serverC.cpp -o serverC
	g++ -g -Wall serverEE.cpp -o serverEE
	g++ -g -Wall serverCS.cpp -o serverCS

client: client.cpp
serverM: serverM.cpp
serverC: serverC.cpp
serverEE: serverEE.cpp
serverCS: serverCS.cpp

.PHONY: clean
clean:
	rm -rf client serverM serverC serverEE serverCS
