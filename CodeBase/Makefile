CXX = g++
CXXFLAGS = -Wall -Werror -Wextra -pedantic -std=c++17 -g -fsanitize=address
LDFLAGS =  -fsanitize=address

EXEC = main

all: main
main: main.o code.o shader.o templates.o ecs.o
	g++ main.o code.o shader.o templates.o ecs.o SOIL/lib/libSOIL.a -lGL -o main -lGLEW -lglfw3 -lX11 -lpthread -lXrandr -ldl 
main.o: main.cpp ./Code/ECSClass.h ./Code/EventClasses.h
	g++  main.cpp -c -lGL  -o main.o -lGLEW -lglfw3 -lX11 -lpthread -lXrandr -ldl
shader.o: ./Code/shaderClass.cpp ./Code/shaderClass.h
	g++ ./Code/shaderClass.cpp -c -lGL  -o shader.o -lGLEW -lglfw3 -lX11 -lpthread -lXrandr -ldl
code.o: ./Code/EventClasses.cpp ./Code/EventClasses.h
	g++ ./Code/EventClasses.cpp -c -o code.o
ecs.o: ./Code/ECSClass.cpp ./Code/ECSClass.h
	g++ ./Code/ECSClass.cpp -c -o ecs.o
templates.o: ./Code/SpriteTemplate.cpp ./Code/ECSClass.h
	g++ ./Code/SpriteTemplate.cpp -c -lGL -o templates.o -lGLEW -lglfw3 -lX11 -lpthread -lXrandr -ldl
clean:
	rm -rf *.o main
