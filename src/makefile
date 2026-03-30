# What to use for compiling.
CXX      := g++

# Get the FLTK flags.
FLTK_CXXFLAGS := $(shell fltk-config --cxxflags)
FLTK_LDFLAGS  := $(shell fltk-config --ldflags --use-images)

# Combine flags.
CXXFLAGS := -std=c++23 -Wall -Wextra $(FLTK_CXXFLAGS) -Iwhisper.cpp/include -Iwhisper.cpp/ggml/include
# Combine libs.
LIBS := $(FLTK_LDFLAGS) -lstdc++exp -lcurl -lportaudio \
        -Lwhisper.cpp/build/src -lwhisper \
        -Lwhisper.cpp/build/ggml/src -lggml -lggml-base -lggml-cpu \
        -Wl,-rpath,whisper.cpp/build/src \
        -Wl,-rpath,whisper.cpp/build/ggml/src \
        -lm
# Set the target file name, or file.
TARGET   := translator
# Specify which files to use.
SRC      := main.cpp api.cpp settings.cpp callbacks.cpp speech_to_text.cpp

all: $(TARGET)

# Compile, but check if $(SRC) exists.
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

# Running make clean runs the below code:
clean:
	rm -f $(TARGET)
