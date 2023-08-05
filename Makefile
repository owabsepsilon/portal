CC = g++
CFLAGS = -g
LDFLAGS = -lreadline
SOURCES = main.cpp
EXECUTABLE = portal

$(EXECUTABLE): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(EXECUTABLE)
