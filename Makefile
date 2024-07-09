
CXX = g++
CXXFLAGS = -std=c++20 -I/usr/local/include/botan-3 -I/usr/include/sqlite3
LDFLAGS = -L/usr/local/lib
LIBS = -lboost_system -lboost_filesystem -lboost_serialization -lsqlite3 -lbotan-3

SRCS = src/denim.cpp
TARGET = denim

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS) $(LIBS)

clean: rm -f $(TARGET)

.PHONY: all clean
