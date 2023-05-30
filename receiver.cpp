#include "local.h"

using namespace std;
int numOfColumns, numOfRows;
static struct MEMORY *sharedMemory;
static struct NUM_OF_READERS *readers;
int shmid, r_shmid, mut_semid, r_semid, w_semid;

static struct sembuf acquire = {0, -1, SEM_UNDO},
                     release = {0, 1, SEM_UNDO};

void openSharedMemory();
void openSemaphores();
void decode(string, string[]);
void writeToFile(char columns[][MAX_STRING_LENGTH]);
void finishSignalCatcher(int);

int main()
{
    cout << "***************IM IN RECEIVER*************" << endl;

    if (sigset(SIGUSR1, finishSignalCatcher) == SIG_ERR) 
    {
        perror("SIGUSR1 handler");
        exit(6);
    }
    openSharedMemory();
    openSemaphores();
    srand(getpid());
    char cols[numOfColumns][MAX_STRING_LENGTH];
    bool isExist[numOfColumns] = {false};

    unsigned col;
    int cntCols = 0;
    // Access and modify the shared matrix, to get all columns
    while (cntCols < numOfColumns)
    {

        col = rand() % numOfColumns;

        // Block SIGUSR1
        sigset_t signalSet;
        sigemptyset(&signalSet);
        sigaddset(&signalSet, SIGUSR1);
        sigprocmask(SIG_BLOCK, &signalSet, NULL);

        acquire.sem_num = col;
        release.sem_num = col;
        if (semop(w_semid, &acquire, 1) == -1)
        {
            perror("RECEIVER: semop write sem");
            exit(3);
        }
        if (semop(mut_semid, &acquire, 1) == -1)
        {
            perror("RECEIVER: semop mut sem");
            exit(3);
        }
        readers->readers[col]++;

        if (semop(mut_semid, &release, 1) == -1)
        {
            perror("RECEIVER: semop read sem");
            exit(3);
        }
        if (readers->readers[col] == 1)
        {
            if (semop(r_semid, &acquire, 1) == -1)
            {
                perror("RECEIVER: semop read sem");
                exit(3);
            }
        }

        if (semop(w_semid, &release, 1) == -1)
        {
            perror("RECEIVER: semop write sem");
            exit(3);
        }
 
        stringstream sline(sharedMemory->data[col]);
        if (sline.good())
        {
            string colNumStr;
            getline(sline, colNumStr, ' ');
            int colNum = stoi(colNumStr);
            if (!isExist[colNum - 1])
            {
                cntCols++;
                isExist[colNum - 1] = true;

                strncpy(cols[colNum - 1], sharedMemory->data[col], MAX_STRING_LENGTH - 1);
                cols[colNum - 1][MAX_STRING_LENGTH - 1] = '\0';
                cout << "Message in RECEIVER: " << sharedMemory->data[col] << endl;
            }
        }

        // done reading

        if (semop(mut_semid, &acquire, 1) == -1)
        {
            perror("RECEIVER: semop mut sem");
            exit(3);
        }
        readers->readers[col]--;

        if (semop(mut_semid, &release, 1) == -1)
        {
            perror("RECEIVER: semop mut sem");
            exit(3);
        }

        if (readers->readers[col] == 0)
        {
            if (semop(r_semid, &release, 1) == -1)
            {
                perror("RECEIVER: semop read sem");
                exit(3);
            }
        }

        // Unblock SIGUSR1
        sigprocmask(SIG_UNBLOCK, &signalSet, NULL);

        // Check for and handle pending signals
        sigset_t pendingSet;
        sigpending(&pendingSet);
        if (sigismember(&pendingSet, SIGUSR1))
        {
            printf("Handling pending SIGUSR1 signal...\n");
            cout << "Signal recived" << endl;
            shmdt(sharedMemory);
            exit(0);
        }

        sleep(rand() % 3);
    }

    shmdt(sharedMemory);

    writeToFile(cols);

    // INFORM PARENT
    kill(getppid(), SIGUSR2);

    cout << "RECIVER IS DONE " << endl;

    return 0;
}

void openSharedMemory()
{
    key_t key = ftok(".", MEM_SEED);
    if (key == -1)
    {
        perror("RECEIVER: key generation");
        exit(3);
    }
    if ((shmid = shmget(key, 0, 0)) == -1)
    {
        perror("RECEIVER: shmid");
        exit(3);
    }
    // Attach the shared memory segment
    if ((sharedMemory = (struct MEMORY *)shmat(shmid, NULL, 0)) == (struct MEMORY *)-1)
    {
        perror("RECEIVER: shmat");
        exit(3);
    }
    numOfColumns = sharedMemory->numOfColumns;
    numOfRows = sharedMemory->numOfRows;

    key = ftok(".", MEM_NUM_OF_READERS_SEED);
    if (key == -1)
    {
        perror("RECEIVER: key generation");
        exit(3);
    }
    if ((r_shmid = shmget(key, 0, 0)) == -1)
    {
        perror("RECEIVER: shmid");
        exit(3);
    }
    // Attach the shared memory segment
    if ((readers = (struct NUM_OF_READERS *)shmat(r_shmid, NULL, 0)) == (struct NUM_OF_READERS *)-1)
    {
        perror("RECEIVER: shmat");
        exit(3);
    }
}

void openSemaphores()
{
    int *semid[] = {&mut_semid, &r_semid, &w_semid};
    int seeds[] = {SEM_MUT_SEED, SEM_R_SEED, SEM_W_SEED};

    key_t key;
    for (int i = 0; i < 3; i++)
    {
        if ((key = ftok(".", seeds[i])) == -1)
        {
            perror("RECEIVER: semaphore key generation");
            exit(1);
        }
        if ((*semid[i] = semget(key, numOfColumns, 0)) == -1)
        {
            perror("RECEIVER: semget obtaining semaphore");
            exit(2);
        }
    }
}

void decode(string encodedColumn, string decodedRows[])
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
                        c = (temp * 26 + c) % 26;
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
                        c = (temp * 26 + c) % 26;
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
        words.push_back(decodedStr);
    }
}

void writeToFile(char columns[][MAX_STRING_LENGTH])
{
    string decodedRows[numOfRows];
    for (int i = 0; i < numOfColumns; i++)
    {
        decode(columns[i], decodedRows);
    }

    ofstream receiverFile;
    receiverFile.open("receiver.txt");

    for (int i = 0; i < numOfRows; i++)
    {
        receiverFile << decodedRows[i] << "\n";
        cout << "RECEIVER decoding" << decodedRows[i] << endl;
    }

    receiverFile.close();
}


void finishSignalCatcher(int signum)
{
    shmdt(sharedMemory);
    exit(signum);
}