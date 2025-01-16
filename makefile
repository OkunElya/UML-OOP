TARGET = main.exe
CXX = clang++
CXXFLAGS= -std=c++23 -Wall --debug
SRC = ./implementation/main.cpp
OBJ=$(SRC:.cpp=.o)

all: $(TARGET)
$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET)

%.o: %cpp$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
ifeq ($(OS),Windows_NT)
	del /Q $(OBJ) $(TARGET).exe 2>nul
else
	rm -f $(OBJ) $(TARGET)
endif