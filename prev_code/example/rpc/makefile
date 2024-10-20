ALL: discovery registry
discovery:discovery.cc main.pb.cc
	g++ -std=c++17 $^ -o $@ -lspdlog -lfmt -lgflags -letcd-cpp-api -lcpprest -lbrpc -lleveldb  -lssl -lcrypto -lprotobuf

registry:registry.cc main.pb.cc
	g++ -std=c++17 $^ -o $@ -lspdlog -lfmt -lgflags -letcd-cpp-api -lcpprest -lbrpc -lleveldb  -lssl -lcrypto -lprotobuf
.PHONY:clean
clean:
	rm discovery registry