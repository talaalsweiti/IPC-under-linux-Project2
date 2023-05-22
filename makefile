CC = g++ # specify the used compiler
CFLAGS = -Wall # specify the options added on compile.
# -Wall -> allow all warnings

all: parent 

# compile parent.cpp
parent: parent.cpp
	$(CC) $(CFLAGS) parent.cpp -o parent

