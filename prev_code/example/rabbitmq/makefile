all:publish consume
publish:publish.cc
	g++ -g -std=c++17 $^ -o $@ -lamqpcpp -lev -lgflags -lfmt -lspdlog
consume:consume.cc
	g++ -g -std=c++17 $^ -o $@ -lamqpcpp -lev -lgflags -lfmt -lspdlog

.PHONY:clean
clean:
	rm -f publish consume