test : main.cc chat_session_member-odb.cxx user-odb.cxx
	c++  $^ -o $@ -I../../../odb/ -lodb-mysql -I./ -lodb -lodb-boost -lspdlog -lfmt -lgflags