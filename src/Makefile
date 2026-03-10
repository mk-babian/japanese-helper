CXX      := g++

FLTK_CXXFLAGS := $(shell fltk-config --cxxflags)
FLTK_LDFLAGS  := $(shell fltk-config --ldflags)

CXXFLAGS := -std=c++23 -Wall -Wextra $(FLTK_CXXFLAGS)
LIBS     := $(FLTK_LDFLAGS) -lstdc++exp -lcurl -Wl,--subsystem,console
TARGET   := translator
SRC      := main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET).exe

