CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lraylib -framework IOKit -framework Cocoa -framework OpenGL -framework CoreServices -framework CoreFoundation -framework AppKit

all: clean format build

build: main.cpp SearchEngine.cpp MacUtils.mm
	$(CXX) $(CXXFLAGS) main.cpp SearchEngine.cpp MacUtils.mm -o maxsearch $(LDFLAGS)

format:
	clang-format -i *.cpp

clean:
	rm -f main maxsearch
