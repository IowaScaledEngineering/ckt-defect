# The name of your output file
TARGET = test

# Compiler to use
CXX = g++

# Compiler flags:
# -Wall: enables all warnings
# -I.: includes the current directory for header files
CXXFLAGS = -Wall -I. -I../ckt-defect/src/menu -std=c++20

# The source file(s)
SRCS = test.cpp display-term.cpp ../ckt-defect/src/menu/menu-mgr.cpp ../ckt-defect/src/menu/menu.cpp

# Default rule
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

# Rule to clean up files
clean:
	rm -f $(TARGET)
