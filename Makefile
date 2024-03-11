## build TinyMOD
## jbs - paragonsoft

all: tinymod

tinymod: tinymod.cpp
##	g++ -o tinymod tinymod.cpp
	g++ -o tinymod `pkg-config --libs alsa` tinymod.cpp -lm -L . -l:libportaudio.a
clean:
	rm tinymod
