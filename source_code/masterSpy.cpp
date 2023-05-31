#include "local.h"

using namespace std;

/* Add functions' prototypes */
void killSpies(int);
void addSignalCatcher();
void decode(string, string[]);
void writeToFile(char columns[][MAX_STRING_LENGTH]);
void openMessageQueue();

/* Define needed variables */
int numOfColumns, numOfRows;
int mid;
int NUM_OF_SPIES;
MESSAGE msg;
pid_t *spies;

int main(int argc, char *argv[])
{
    cout << "***************IM IN MASTER SPY*************" << endl;

    if (argc != 4)
    {
        printf("Usage: %s numOfSpies numOfColumns numOfRows\n", argv[0]);
        exit(1);
    }

    /* Save passed arguments in a file */
    NUM_OF_SPIES = atoi(argv[1]);
    numOfColumns = atoi(argv[2]);
    numOfRows = atoi(argv[3]);

    addSignalCatcher();

    openMessageQueue();

    char cols[numOfColumns][MAX_STRING_LENGTH];
    bool isExist[numOfColumns] = {false};

    /* Define spies array */
    spies = new pid_t[NUM_OF_SPIES];

    /* Fork spies */
    for (int i = 0; i < NUM_OF_SPIES; i++)
    {
        spies[i] = createProcesses("./spy");
    }

    int n, cntCols = 0;

    /* Access the shared matrix, to get all columns */
    while (cntCols < numOfColumns)
    {
        /* read message from queue (coming from a spy) */
        memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));
        if ((n = msgrcv(mid, &msg, BUFSIZ, getpid(), 0)) == -1)
        {
            perror("Master Spy: reading msg from queue");
            exit(10);
        }

        /* extract column number and check of column was previously retrieved */
        stringstream sline(msg.buffer);
        if (sline.good())
        {
            string colNumStr;
            getline(sline, colNumStr, ' ');
            int colNum = stoi(colNumStr);
            if (!isExist[colNum - 1]) /* add column and send info to opengl to show it in UI */
            {
                cntCols++;
                isExist[colNum - 1] = true;
                strncpy(cols[colNum - 1], msg.buffer, MAX_STRING_LENGTH - 1);
                cols[colNum - 1][MAX_STRING_LENGTH - 1] = '\0';
                cout << " Message in MASTER SPY: " << msg.buffer << endl;
                strcpy(msg.buffer, colNumStr.c_str());
                msg.msg_to = 2;
                msgsnd(mid, &msg, strlen(msg.buffer), 0);
            }
        }
    }

    /* kill all spies if retrieved all columns */
    for (int i = 0; i < NUM_OF_SPIES; i++)
    {
        kill(spies[i], SIGUSR1);
    }

    writeToFile(cols); /* Write decoded columns to file */

    cout << "MASTER SPY FINISHED" << endl;
    kill(getppid(), SIGUSR1); /* send signal that master spy finished decoding */

    return 0;
}

/* This function opens the message queue for receving columns from spies */
void openMessageQueue()
{
    key_t key;
    if ((key = ftok(".", Q_SEED)) == -1) /* Generate key */
    {
        perror("Master Spy:: key generation");
        exit(1);
    }

    if ((mid = msgget(key, 0)) == -1) /* open shared memory */
    {
        perror("Master Spy:: Queue openning");
        exit(2);
    }
}

/* This function decodes the received column and saves it in its correct index */
void decode(string encodedColumn, string decodedRows[])
{
    stringstream sColumn(encodedColumn);
    vector<string> words;
    int col;

    /* Read the column number from column string and save it */
    if (sColumn.good())
    {
        string substr;
        getline(sColumn, substr, ' ');
        col = stoi(substr);
        words.push_back(substr);
    }

    int row = 0;
    while (sColumn.good())
    {
        string substr;
        getline(sColumn, substr, ' '); /* get word */

        string decodedStr = "";
        int charNum = 0;
        for (unsigned i = 0; i < substr.length(); i++)
        {
            charNum++;
            char c = substr[i];
            if (isalpha(c)) /* if letter -> subtract specified number mod 26 */
            {
                if (isupper(c))
                {
                    c -= 'A';
                    c = (c - ((charNum)*col));
                    if (c < 0)
                    {
                        int temp = ((-1 * c / 26) + 1);
                        c = (temp * 26 + c) % 26;
                    }
                    c += 'A';
                }
                else
                {
                    c -= 'a';
                    c = (c - ((charNum)*col));
                    if (c < 0)
                    {
                        int temp = ((-1 * c / 26) + 1);
                        c = (temp * 26 + c) % 26;
                    }
                    c += 'a';
                }
                decodedStr += c;
            }
            else if (isdigit(c)) /* if digit -> number or special character */
            {
                if (c == '1' && i + 1 < substr.length() && substr[i + 1] == '0') /* 1000000 for the number 0 */
                {
                    decodedStr += "0";
                    i += 6;
                    continue;
                }
                if (c == '9' && i + 5 < substr.length()) /* number from 1 to 9 */
                {

                    int temp = substr[i + 5] - '0';
                    temp = 10 - temp;
                    decodedStr += to_string(temp);
                    i += 5;
                    continue;
                }

                switch (c) /* Special character -> substitute with suitable character */
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

        /* Don't add "alright" to decoded file */
        if (decodedStr == "alright")
        {
            row++;
            continue;
        }

        if (col != 1)
        {
            decodedRows[row] += " ";
        }

        /* add decoded string to suitable spot in array */
        decodedRows[row] += decodedStr;
        row++;
        words.push_back(decodedStr);
    }
}

/* This function calls for decoding retreived rows and writes result to spy.txt */
void writeToFile(char columns[][MAX_STRING_LENGTH])
{
    string decodedRows[numOfRows];
    /* Decode all columns */
    for (int i = 0; i < numOfColumns; i++)
    {
        decode(columns[i], decodedRows);
    }

    /* Write decoded columns to file in the correct way */
    ofstream spyFile;
    spyFile.open("../outputs/spy.txt");

    for (int i = 0; i < numOfRows; i++)
    {
        spyFile << decodedRows[i] << "\n";
        cout << "SPY deocding: " << decodedRows[i] << "\n";
    }

    spyFile.close();
}

/* This function adds the needed signal catcher */
void addSignalCatcher()
{
    if (sigset(SIGUSR2, killSpies) == SIG_ERR)
    {
        perror("SIGUSR2 handler");
        exit(4);
    }
}

/* This function kills the spies and exits from process */
void killSpies(int signum)
{
    for (int i = 0; i < NUM_OF_SPIES; i++)
    {
        kill(spies[i], SIGUSR1);
    }
    delete[] spies;
    exit(signum);
}