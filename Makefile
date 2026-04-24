# =================== TinyMOD Makefile ===================
# Build configuration for TinyMOD MOD player

# === Compiler Settings ===
CC = g++
CFLAGS = -O2 -Wall -Wextra

# === Source Files ===
SOURCES = src/main.cpp src/paula.cpp src/modplayer.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = tinymod

# === Libraries ===
# PortAudio library (static link)
LIBS = -L. -l:libportaudio.a -lm

# === PortAudio Configuration ===
# Optional: link with ALSA for Linux
PORTAUDIO_LIBS = $(shell pkg-config --libs alsa 2>/dev/null)
LIBS += $(PORTAUDIO_LIBS)

# === Build Rules ===
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LIBS)
	@echo "Build complete: $(TARGET)"

# Compile source files to object files
src/%.o: src/%.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete"

# Phony targets (not actual files)
.PHONY: all clean
