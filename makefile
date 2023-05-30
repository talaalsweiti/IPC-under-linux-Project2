CC = g++ # specify the used compiler
CFLAGS = -Wall # specify the options added on compile.
# -Wall -> allow all warnings


all: opengl parent child helper sender receiver masterSpy spy #readerTest  

# compile parent.cpp
parent: parent.cpp
	$(CC) $(CFLAGS) parent.cpp -o parent

# compile child.cpp
child: child.cpp
	$(CC) $(CFLAGS) child.cpp -o child
	
# compile helper.cpp
helper: helper.cpp
	$(CC) $(CFLAGS) helper.cpp -o helper
	
# compile spy.cpp
spy: spy.cpp
	$(CC) $(CFLAGS) spy.cpp -o spy

# compile masterSpy.cpp
masterSpy: masterSpy.cpp
	$(CC) $(CFLAGS) masterSpy.cpp -o masterSpy

# compile sender.cpp
sender: sender.cpp
	$(CC) $(CFLAGS) sender.cpp -o sender

# compile receiver.cpp
receiver: receiver.cpp
	$(CC) $(CFLAGS) receiver.cpp -o receiver

# compile readerTest.cpp
readerTest: readerTest.cpp
	$(CC) $(CFLAGS) readerTest.cpp -o readerTest

opengl: opengl.cpp 
	g++ opengl.cpp -o opengl -lglut -lGLU -lGL -lfreetype -I./resources/freetype2 -I./resources/libpng16
