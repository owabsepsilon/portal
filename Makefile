CC = g++
CFLAGS = -g
LDFLAGS = -lreadline -lncurses -lcrypto
SOURCES = main.cpp
EXECUTABLE = shell

$(EXECUTABLE): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(EXECUTABLE)
