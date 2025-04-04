CC = gcc
CFLAGS = -g -Wall

OBJS = proxy_parse.o proxy_server_with_cache.o
TARGET = proxy

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lpthread

proxy_parse.o: proxy_parse.c proxy_parse.h
	$(CC) $(CFLAGS) -c proxy_parse.c

proxy_server_with_cache.o: proxy_server_with_cache.c
	$(CC) $(CFLAGS) -c proxy_server_with_cache.c

clean:
	rm -f $(TARGET) *.o

tar:
	tar -cvzf ass1.tgz proxy_server_with_cache.c README Makefile proxy_parse.c proxy_parse.h
