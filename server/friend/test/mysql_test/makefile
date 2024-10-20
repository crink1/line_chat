CFLAGS = -I../../../odb/ -I../../../common -I./ 
main : main.cc relation-odb.cxx friend_apply-odb.cxx chat_session-odb.cxx chat_session_member-odb.cxx
	g++ -std=c++17 $^ -o $@   -lodb-mysql $(CFLAGS) -lodb -lodb-boost -lfmt -lspdlog -lgflags