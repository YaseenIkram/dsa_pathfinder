CXX = g++
CXXFLAGS = -std=c++17 -Wall -MMD -MP

# The SFML libraries required to draw windows and graphics
SFML_LIBS = -lsfml-graphics -lsfml-window -lsfml-system

# All the source files we need to compile
SRCS = main.cpp MinHeap.cpp Grid.cpp Pathfinder.cpp MazeLibrary.cpp
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

# The name of our final application
EXEC = pathfinder

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $(EXEC) $(SFML_LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(OBJS) $(DEPS) $(EXEC)