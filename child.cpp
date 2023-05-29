#include "local.h"

using namespace std;

pid_t cPid;
key_t key;
int mid, n;
MESSAGE msg;
int col;

void readMessage();
string encode();
void writeColToSharedMem(string);

int main(int argc, char *argv[])
{

    cout << "***************IM IN CHILD*************" << endl;

    readMessage();
    // cout << "CHILD:: MSG: " << msg.buffer << endl;

    string encodedColumn = encode();

    // cout << "CHILD:: ENCODED: " << encodedColumn << endl;

    writeColToSharedMem(encodedColumn);

    // decode(encodedColumn);
    return 0;
}

void readMessage()
{
    if ((key = ftok(".", Q_SEED)) == -1)
    {
        perror("Child: key generation");
        exit(1);
    }

    if ((mid = msgget(key, 0)) == -1)
    {
        perror("Child: Queue openning");
        exit(2);
    }
    cPid = getpid();
    memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));

    if ((n = msgrcv(mid, &msg, BUFSIZ, cPid, 0)) == -1)
    {
        perror("Child: reading msg from queue");
        exit(3);
    }
}

// return space delimited encoded column
string encode()
{
    stringstream sColumn(msg.buffer);
    string encodedString = "";
    if (sColumn.good())
    {
        string substr;
        getline(sColumn, substr, ' ');
        col = stoi(substr);
        encodedString += substr;
    }
    while (sColumn.good())
    {
        string substr;
        getline(sColumn, substr, ' ');

        string encodedWord = "";
        for (unsigned i = 0; i < substr.length(); i++)
        {
            char c = substr[i];
            if (isalpha(c))
            {
                if (isupper(c))
                {
                    c -= 'A';
                    c = (c + ((i + 1) * col)) % 26;
                    c += 'A';
                }
                else
                {
                    c -= 'a';
                    c = (c + ((i + 1) * col)) % 26;
                    c += 'a';
                }
                encodedWord += c;
            }
            else if (isdigit(c))
            {
                int temp = 1000000 - (c - '0');
                encodedWord += to_string(temp);
            }
            else
            {
                switch (c)
                {
                case '!':
                    encodedWord += "1";
                    break;
                case '?':
                    encodedWord += "2";
                    break;
                case ',':
                    encodedWord += "3";
                    break;
                case '*':
                    encodedWord += "4";
                    break;
                case ':':
                    encodedWord += "5";
                    break;
                case '%':
                    encodedWord += "6";
                    break;
                case '.':
                    encodedWord += "7";
                    break;
                default:
                    encodedWord += c;
                }
            }
        }
        encodedString += " " + encodedWord;
    }
    return encodedString;
}

void writeColToSharedMem(string str)
{
    int shmid;
    struct MEMORY *sharedMemory;
    // generate key for the sahred memory
    if ((key = ftok(".", MEM_SEED)) == -1)
    {
        perror("CHILD: shared memory key generation");
        exit(1);
    }

    // open the shared memory
    if ((shmid = shmget(key, 0, 0)) == -1)
    {
        perror("shmid");
        exit(3);
    }

    // attach shared memory
    if ((sharedMemory = (struct MEMORY *)shmat(shmid, NULL, 0)) == (struct MEMORY *)-1)
    {
        perror("shmptr -- parent -- attach");
        exit(1);
    }

    // TODO:: should I add semaphores here?

    strncpy(sharedMemory->data[col - 1], str.c_str(), MAX_STRING_LENGTH - 1);
    sharedMemory->data[col - 1][MAX_STRING_LENGTH - 1] = '\0';

    cout << "CHILD: ENCODED: " << sharedMemory->data[col - 1] << endl;

    shmdt(sharedMemory);
}