# Define the compiler
CXX = g++

# Define the compiler flags
CXXFLAGS = -Wall -Wextra -std=c++17

# Define the target executable
TARGET = myapp

# Define source files
SOURCES = main.cpp peer.cpp

# Define header files
HEADERS = peer.h

# Define object files
OBJECTS = $(SOURCES:.cpp=.o)

# The first rule is the one executed when no parameters are fed to the Makefile
all: $(TARGET)

# Link the object files into the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Compile the source files into object files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $<

# Clean up
clean:
	rm -f $(TARGET) $(OBJECTS)

# Phony targets
.PHONY: all clean