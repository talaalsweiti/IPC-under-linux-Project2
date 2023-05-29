#include "local.h"

using namespace std;
int numOfColumns, numOfRows;
int mid;
int NUM_OF_SPIES;
MESSAGE msg;
void killSpies(int);
void signalCatcher();
void decode(string, string[]);
void writeToFile(char columns[][MAX_STRING_LENGTH]);
void openMessageQueue();
bool flag = true; // will change when the parent sends a signal indicating that the reciever has finished
pid_t  *spies;



int main(int argc, char *argv[])
{
    cout << "***************IM IN MASTER SPY*************" << endl;

    pid_t pid = getpid();
    if (argc != 4)
    {
        printf("Usage: %s numOfSpies numOfColumns numOfRows\n", argv[0]);
        exit(1);
    }

    NUM_OF_SPIES = atoi(argv[1]);
    numOfColumns = atoi(argv[2]);
    numOfRows = atoi(argv[3]);

    signalCatcher();

    openMessageQueue();

    char cols[numOfColumns][MAX_STRING_LENGTH];
    bool isExist[numOfColumns] = {false};

    spies = new pid_t[NUM_OF_SPIES];


    for (int i = 0; i < NUM_OF_SPIES; i++)
    {
        spies[i] = createProcesses("./spy");
    }

    int n, cntCols = 0;
    // Access and modify the shared matrix, to get all columns
    while (flag && cntCols < numOfColumns)
    { // read message from queue
        memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));

        if ((n = msgrcv(mid, &msg, BUFSIZ, pid, 0)) == -1)
        {
            perror("Master Spy: reading msg from queue");
            exit(10);
        }

        stringstream sline(msg.buffer);
        if (sline.good())
        {
            string colNumStr;
            getline(sline, colNumStr, ' ');
            int colNum = stoi(colNumStr);
            if (!isExist[colNum - 1])
            {
                cntCols++;
                isExist[colNum - 1] = true;
                strncpy(cols[colNum - 1], msg.buffer, MAX_STRING_LENGTH - 1);
                cols[colNum - 1][MAX_STRING_LENGTH - 1] = '\0';
                cout << "Column number: " << colNum << " -- MEssage : " << msg.buffer << endl;
                cout << "Master Spy READING DONE " << endl;
            }
        }
    }

    for (int i = 0; i < NUM_OF_SPIES; i++)
    {
        kill(spies[i], SIGUSR1);
    }
    cout << "MASTER SPY FINISHED" << endl;
    
    kill(getppid(), SIGUSR1);

    writeToFile(cols);

    // shmctl(shmid, IPC_RMID, (struct shmid_ds *)0);
    // Other processes can now attach to the same shared memory segment using the same key
    return 0;
}

void openMessageQueue()
{
    key_t key;
    if ((key = ftok(".", Q_SEED)) == -1)
    {
        perror("Master Spy:: key generation");
        exit(1);
    }

    if ((mid = msgget(key, 0)) == -1)
    {
        perror("Master Spy:: Queue openning");
        exit(2);
    }
}

void decode(string encodedColumn, string decodedRows[])
{
    // cout << "WEEEEEEEEEEEE " << encodedColumn << endl;
    stringstream sColumn(encodedColumn);
    int col;
    if (sColumn.good())
    {
        string substr;
        getline(sColumn, substr, ' ');
        col = stoi(substr);
    }

    int row = 0;
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
            else
            {
                decodedStr += c;
            }
        }
        if (decodedStr == "alright")
        {
            row++;
            continue;
        }
        if (col != 1)
        {
            decodedRows[row] += " ";
        }
        decodedRows[row] += decodedStr;
        row++;
    }
}

void writeToFile(char columns[][MAX_STRING_LENGTH])
{
    // cout << "ZFFTTT" << endl;
    string decodedRows[numOfRows];
    // cout << "ROWS:: " << numOfRows << endl;
    for (int i = 0; i < numOfColumns; i++)
    {
        decode(columns[i], decodedRows);
        // cout << "ZFFTTT2" << endl;
    }

    ofstream spyFile;
    spyFile.open("spy.txt");

    for (int i = 0; i < numOfRows; i++)
    {
        spyFile << decodedRows[i] << "\n";
        cout << "SPY deocding" << decodedRows[i] << "\n";
    }

    spyFile.close();
}

void signalCatcher()
{
    if (sigset(SIGUSR2, killSpies) == SIG_ERR)
    {
        perror("SIGUSR2 handler");
        exit(4);
    }
}

void killSpies(int signum)
{
   for (int i = 0; i < NUM_OF_SPIES; i++)
    {
        kill(spies[i], SIGUSR1);
    }
    kill(getpid(),9);
}