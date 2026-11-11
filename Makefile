CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -I./include
WIN_LIBS = -L./lib -lglfw3 -lopengl32 -lgdi32

win:
	$(CXX) $(CXXFLAGS) ./src/main.cpp ./src/glad.c -o ./build/SmartVillage.exe $(WIN_LIBS)
	./build/SmartVillage.exe

linux:
	$(CXX) $(CXXFLAGS) ./src/main.cpp ./src/glad.c -o ./build/SmartVillage -lglfw -lGL -ldl -pthread
	./build/SmartVillage
