#include "local.h"

using namespace std;

pid_t cPid;
key_t key;
int mid, n;
MESSAGE msg;
int col;

void readMessage();
void decode(string);
string encode();
void writeColToSharedMem(string);

int main(int argc, char *argv[])
{
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
    cout << "CHILD:: ENCODED: " << sharedMemory->data[col - 1] << endl;

    shmdt(sharedMemory);
}

void decode(string encodedColumn)
{
    stringstream sColumn(encodedColumn);
    vector<string> words;
    int col;
    if (sColumn.good())
    {
        string substr;
        getline(sColumn, substr, ' ');
        col = stoi(substr);
        words.push_back(substr);
    }

    while (sColumn.good())
    {
        string substr;
        getline(sColumn, substr, ' ');

        string decodedStr = "";
        for (unsigned i = 0; i < substr.length(); i++)
        {
            char c = substr[i];
            if (isalpha(c))
            {
                if (isupper(c))
                {
                    c -= 'A';
                    c = (c - ((i + 1) * col));
                    if (c < 0)
                    {
                        int temp = ((-1 * c / 26) + 1);
                        c = temp * 26 + c;
                    }
                    c += 'A';
                }
                else
                {
                    c -= 'a';
                    c = (c - ((i + 1) * col));
                    if (c < 0)
                    {
                        int temp = ((-1 * c / 26) + 1);
                        c = temp * 26 + c;
                    }
                    c += 'a';
                }
                decodedStr += c;
            }
            else if (isdigit(c))
            {
                if (c == '1' && i + 1 < substr.length() && substr[i + 1] == '0')
                {
                    decodedStr += "0";
                    i += 6;
                    continue;
                }
                if (c == '9' && i + 5 < substr.length())
                {

                    int temp = substr[i + 5] - '0';
                    temp = 10 - temp;
                    decodedStr += to_string(temp);
                    i += 5;
                    continue;
                }

                switch (c)
                {
                case '1':
                    decodedStr += "!";
                    break;
                case '2':
                    decodedStr += "?";
                    break;
                case '3':
                    decodedStr += ",";
                    break;
                case '4':
                    decodedStr += "*";
                    break;
                case '5':
                    decodedStr += ":";
                    break;
                case '6':
                    decodedStr += "%";
                    break;
                case '7':
                    decodedStr += ".";
                    break;
                default:
                    decodedStr += c;
                }
            }
        }
        words.push_back(decodedStr);
        cout << substr << "::" << decodedStr << " ";
    }
    cout << endl;
}