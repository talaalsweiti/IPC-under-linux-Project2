CC = g++ # specify the used compiler
CFLAGS = -Wall # specify the options added on compile.
# -Wall -> allow all warnings

all: parent child helper readerTest

# compile parent.cpp
parent: parent.cpp
	$(CC) $(CFLAGS) parent.cpp -o parent

# compile child.cpp
child: child.cpp
	$(CC) $(CFLAGS) child.cpp -o child
	
# compile helper.cpp
helper: helper.cpp
	$(CC) $(CFLAGS) helper.cpp -o helper

# compile readerTest.cpp
readerTest: readerTest.cpp
	$(CC) $(CFLAGS) readerTest.cpp -o readerTest

