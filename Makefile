all:
	rm -rf client server
	gcc -pthread client.c -o client
	gcc -pthread server.c -o server 
clean:
	rm -rf client server