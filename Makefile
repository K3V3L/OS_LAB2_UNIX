all: clean compile run
compile: source.cpp
	g++ -Wall -pthread source.cpp -o main 
run: main
	./main
runsort1: main
	./main pid 
runsort2: main
	./main cpu 
runsort3: main
	./main name
clean: 
	mv main mainbak
