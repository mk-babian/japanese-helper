CXX      := g++

FLTK_CXXFLAGS := $(shell fltk-config --cxxflags)
FLTK_LDFLAGS  := $(shell fltk-config --ldflags)

CXXFLAGS := -std=c++23 -Wall -Wextra $(FLTK_CXXFLAGS)
LIBS     := $(FLTK_LDFLAGS) -lstdc++exp -lcurl
TARGET   := translator
SRC      := main.cpp api.cpp settings.cpp callbacks.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET).exe

