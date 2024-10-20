all : server client
server:server.cc main.pb.cc
	g++ -std=c++17 $^ -o $@ -lbrpc -lleveldb -lgflags -lssl -lcrypto -lprotobuf 
client:client.cc main.pb.cc
	g++ -std=c++17 $^ -o $@ -lbrpc -lleveldb -lgflags -lssl -lcrypto -lprotobuf 

.PHONY:clean
clean:
	rm server client