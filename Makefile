build:
	g++ server.cpp -Wall -g -O3 -o server

run:
	./server

clean:
	rm -rf server