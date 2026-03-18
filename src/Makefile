CXX      := g++

FLTK_CXXFLAGS := $(shell fltk-config --cxxflags)
FLTK_LDFLAGS  := $(shell fltk-config --ldflags --use-images)

CXXFLAGS := -std=c++23 -Wall -Wextra $(FLTK_CXXFLAGS)
LIBS     := $(FLTK_LDFLAGS) -lstdc++exp -lcurl -Wl,--subsystem,console
TARGET   := translator
SRC      := main.cpp api.cpp settings.cpp callbacks.cpp

all: $(TARGET)

$(TARGET): $(SRC) resources.o
	$(CXX) $(CXXFLAGS) $(SRC) resources.o -o $(TARGET) $(LIBS)

resources.o: ../app_icon.ico resources.rc
	windres resources.rc -o resources.o

clean:
	rm -f $(TARGET).exe resources.o
