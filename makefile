BIN=WebGobang
LDPATH=-L /usr/lib64/mysql
LDFlag=-lpthread -ljsoncpp -lmysqlclient -lcrypto

$(BIN):webgobang.cpp
	g++ $^ -o $@ $(LDPATH) $(LDFlag)

.PHONY:clean
clean:
	rm $(BIN)
