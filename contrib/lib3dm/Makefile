OBJECTS = opennurbs_crc.o 3dm.o
CFLAGS  = -m64 -O3 -std=c99 -DUNIX -Wall -pedantic -funroll-loops -ffast-math -fomit-frame-pointer

.PHONY: all
all: $(OBJECTS)

.PHONY: clean
clean:
	rm -f *.o

%.o: %.cpp
	gcc -c $(CFLAGS) $<
