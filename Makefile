CC = g++
CFLAGS = -g
LDFLAGS = -lreadline
SOURCES = main.cpp
EXECUTABLE = shell

$(EXECUTABLE): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(EXECUTABLE)
