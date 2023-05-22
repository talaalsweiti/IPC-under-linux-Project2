#include <stdio.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <iostream>
#include <cstdlib>


using namespace std;
void readFile();

int main(int argc, char *argv[])
{
    readFile();
    return 0;
}

void readFile(){
    FILE    *fin;
    string command = "sed -i -e \"s/\\s\\ */\\ /g; /^$/d\" sender.txt";
    fin = popen(command.c_str(), "r");
    pclose(fin);
}