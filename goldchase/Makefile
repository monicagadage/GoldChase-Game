main: main.cpp libmap.a goldchase.h libmap.a
	g++ -std=c++17 main.cpp -o main -L. -lmap -lpanel -lncurses -lpthread -lrt


libmap.a: Screen.o Map.o player.o
	ar -r libmap.a Screen.o Map.o player.o

Screen.o: Screen.cpp
	g++ -std=c++17 -c Screen.cpp

Map.o: Map.cpp
	g++ -std=c++17 -c Map.cpp

player.o: player.cpp
	g++ -std=c++17 -c player.cpp

clean:
	rm -f Screen.o Map.o player.o libmap.a main
