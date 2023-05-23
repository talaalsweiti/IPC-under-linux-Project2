CC = g++ # specify the used compiler
CFLAGS = -Wall # specify the options added on compile.
# -Wall -> allow all warnings

all: parent child

# compile parent.cpp
parent: parent.cpp
	$(CC) $(CFLAGS) parent.cpp -o parent

# compile parent.cpp
child: child.cpp
	$(CC) $(CFLAGS) child.cpp -o child

