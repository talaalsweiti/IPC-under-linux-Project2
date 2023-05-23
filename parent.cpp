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
#include <algorithm>
#include <vector>
#include <fstream>
#include <sstream>
using namespace std;

void readFile();

vector<vector<string>> tokens;
int maxColumns = 0;
int main(int argc, char *argv[])
{
    readFile();
    return 0;
}

void readFile()
{
    FILE *fin;
    string command = "sed -i -e \"s/\\s\\ */\\ /g; /^$/d\" sender.txt";
    if ((fin = popen(command.c_str(), "r")) == NULL)
    {
        perror("Preproccessing sender.txt");
        exit(1);
    }
    pclose(fin);

    ifstream senderFile("sender.txt");
    if (!senderFile.good())
    {
        perror("Open range.txt");
        exit(2);
    }

    string line;
    while (getline(senderFile, line))
    {
        stringstream sline(line);
        vector<string> words;
        while (sline.good())
        {
            string substr;
            getline(sline, substr, ' ');
            words.push_back(substr);
        }
        maxColumns = max(maxColumns,(int)words.size()); 
        tokens.push_back(words);
    }
    for(unsigned i=0;i<tokens.size();i++) {
        while(tokens[i].size() < maxColumns) {
            tokens[i].push_back("alright");
        }
    }
    
}