server.o client.o: server.c client.c 
	gcc server.c -lpthread -o server
	gcc client.c -lpthread -o client

