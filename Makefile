CFLAGS = -O2 -Wall -lm -lasound
CC = cc -g

OBJECTS = analyzer.o fft.o

all:	$(OBJECTS)
	$(CC) $(CFLAGS) -o analyzer $(OBJECTS)
clean:  
	/bin/rm -f $(OBJECTS) analyzer

analyzer.o:     analyzer.c
fft.o:		fft.c
