CXX      ?= c++
CXXFLAGS ?= -std=c++20 -O3 -Wall -Wextra -Wpedantic

TARGET   := sortedintersection
SRC      := sortedintersection.cpp

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
