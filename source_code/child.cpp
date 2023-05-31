/* Child receives a columns from the sender and performs encoding operation */

#include "local.h"

using namespace std;

/* Add functions' prototypes */
void readMessage();
string encode();
void writeColToSharedMem(string);

/* define needed variables */
pid_t cPid;
key_t key;
int mid, n;
MESSAGE msg;
int col;

int main(int argc, char *argv[])
{
    cout << "***************IM IN CHILD*************" << endl;

    readMessage();
    string encodedColumn = encode();
    writeColToSharedMem(encodedColumn);

    return 0;
}

/* This functions reads the column from the message queue */
void readMessage()
{
    if ((key = ftok(".", Q_SEED)) == -1) /* Generate key */
    {
        perror("Child: key generation");
        exit(1);
    }

    if ((mid = msgget(key, 0)) == -1) /* open message queue */
    {
        perror("Child: Queue openning");
        exit(2);
    }

    memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));

    if ((n = msgrcv(mid, &msg, BUFSIZ, getpid(), 0)) == -1) /* receive message from sender process */
    {
        perror("Child: reading msg from queue");
        exit(3);
    }
}

/* This function extract the column from the message and encodes it */
string encode()
{
    stringstream sColumn(msg.buffer);
    string encodedString = "";

    /* Read the column number from column string and save it */
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
        getline(sColumn, substr, ' '); /* get word */

        string encodedWord = "";
        for (unsigned i = 0; i < substr.length(); i++) /* loop the word char by char to encode each char */
        {
            char c = substr[i];
            if (isalpha(c)) /* if letter -> add specified number mod 26 */
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
            else if (isdigit(c)) /* if digit -> subtract from 1000000 */
            {
                int temp = 1000000 - (c - '0');
                encodedWord += to_string(temp);
            }
            else /* char is a special character -> substitute with suitable encoded value */
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
    if ((key = ftok(".", MEM_SEED)) == -1) /* Generate shared memory key */
    {
        perror("CHILD: shared memory key generation");
        exit(1);
    }

    if ((shmid = shmget(key, 0, 0)) == -1) /* get shared memory from key */
    {
        perror("shmid");
        exit(3);
    }

    if ((sharedMemory = (struct MEMORY *)shmat(shmid, NULL, 0)) == (struct MEMORY *)-1) /* attach pointer to shared memory */
    {
        perror("shmptr -- parent -- attach");
        exit(1);
    }

    /* write encoded column to memory */
    strncpy(sharedMemory->data[col - 1], str.c_str(), MAX_STRING_LENGTH - 1);
    sharedMemory->data[col - 1][MAX_STRING_LENGTH - 1] = '\0';

    shmdt(sharedMemory); /* Detach shared memory */
}