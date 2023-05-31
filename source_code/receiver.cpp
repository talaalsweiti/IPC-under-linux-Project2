/* Receiver tries to obtain all columns from memory and decode them before master spy */
#include "local.h"

using namespace std;

/* Add functions' prototypes */
void openMessageQueue();
void openSharedMemory();
void openSemaphores();
void decode(string, string[]);
void writeToFile(char columns[][MAX_STRING_LENGTH]);
void addSignalCatcher();
void finishSignalCatcher(int);

/* Define needed variables */
int numOfColumns, numOfRows;
static struct MEMORY *sharedMemory;
static struct NUM_OF_READERS *readers; /* Define pointer to the shared memory for the number of readers */
int mid, shmid, r_shmid, mut_semid, r_semid, w_semid;
MESSAGE msg;

/* define two struct variables, 'acquire' and 'release', which specify the parameters for acquiring and releasing a semaphore respectively */
static struct sembuf acquire = {0, -1, SEM_UNDO},
                     release = {0, 1, SEM_UNDO};

int main()
{
    cout << "***************IM IN RECEIVER*************" << endl;

    openMessageQueue();
    openSharedMemory();
    openSemaphores();

    srand(getpid()); /* Set spy PID as unique seed for rand */

    char cols[numOfColumns][MAX_STRING_LENGTH]; // to store the columns
    bool isExist[numOfColumns] = {false};

    unsigned col;
    int cntCols = 0;

    // Access the shared matrix, to get all columns
    while (cntCols < numOfColumns)
    {
        col = rand() % numOfColumns; /* Randomly select a column */

        /*
            blocking SIGUSR1 before aquiring the semaphore so
            the spy wouldn't get killed while having the semaphore
        */
        sigset_t signalSet;                       // sigset_t is a data structure used to represent a set of signals.
        sigemptyset(&signalSet);                  // ensures that signalSet is empty before adding any signals
        sigaddset(&signalSet, SIGUSR1);           // adds the SIGUSR1 signal to the signal set
        sigprocmask(SIG_BLOCK, &signalSet, NULL); // prevents the SIGUSR1 signal from being delivered to the process

        /* Acquire mutex and write semaphores to increase the number of readers */
        acquire.sem_num = col;
        release.sem_num = col;

        if (semop(w_semid, &acquire, 1) == -1) /* Aquire write sempahore */
        {
            perror("RECEIVER: semop write sem");
            exit(3);
        }
        if (semop(mut_semid, &acquire, 1) == -1) /* Acquire mutex semaphore */
        {
            perror("RECEIVER: semop mut sem");
            exit(3);
        }

        readers->readers[col]++; /* increment the number of readers for a specific column */

        if (semop(mut_semid, &release, 1) == -1) /* release mutex semaphore */
        {
            perror("RECEIVER: semop read sem");
            exit(3);
        }

        /*
            Checks if the current process is the first reader for the specific column.
            If true, it means that no other readers are currently accessing the column.
        */
        if (readers->readers[col] == 1)
        {
            if (semop(r_semid, &acquire, 1) == -1)
            {
                perror("RECEIVER: semop read sem");
                exit(3);
            }
        }

        if (semop(w_semid, &release, 1) == -1) /* Release write semaphore */
        {
            perror("RECEIVER: semop write sem");
            exit(3);
        }

        /* extract column number and check of column was previously retrieved */
        stringstream sline(sharedMemory->data[col]);
        if (sline.good())
        {
            string colNumStr;
            getline(sline, colNumStr, ' ');
            int colNum = stoi(colNumStr);
            // check if has been processed before
            if (!isExist[colNum - 1]) /* add column and send info to opengl to show it in UI */
            {
                cntCols++;
                isExist[colNum - 1] = true;
                strncpy(cols[colNum - 1], sharedMemory->data[col], MAX_STRING_LENGTH - 1);
                cols[colNum - 1][MAX_STRING_LENGTH - 1] = '\0';
                cout << "Message in RECEIVER: " << sharedMemory->data[col] << endl;
                strcpy(msg.buffer, colNumStr.c_str());
                msg.msg_to = 1;
                msgsnd(mid, &msg, strlen(msg.buffer), 0);
            }
        }

        if (semop(mut_semid, &acquire, 1) == -1) /* Acquire mutex semaphore */
        {
            perror("RECEIVER: semop mut sem");
            exit(3);
        }

        readers->readers[col]--; /* Decrement number of readers */

        if (semop(mut_semid, &release, 1) == -1) /* release mutex semaphore */
        {
            perror("RECEIVER: semop mut sem");
            exit(3);
        }

        if (readers->readers[col] == 0) /* if last reader, release read semaphore */
        {
            if (semop(r_semid, &release, 1) == -1)
            {
                perror("RECEIVER: semop read sem");
                exit(3);
            }
        }

        /* Unblock SIGUSR1 */
        sigprocmask(SIG_UNBLOCK, &signalSet, NULL);

        /* Handle if SIGUSR1 was signaled whle holding the semaphores */
        sigset_t pendingSet;
        sigpending(&pendingSet);
        if (sigismember(&pendingSet, SIGUSR1))
        {
            cout << "Signal recived" << endl;
            shmdt(sharedMemory);
            exit(0);
        }

        sleep(rand() % 3); /* random sleep before getting another column (sleeps less than spy for more advantage) */
    }

    shmdt(sharedMemory);

    writeToFile(cols); /* Write decoded columns to file */

    cout << "RECIVER IS DONE " << endl;

    kill(getppid(), SIGUSR2); /* send signal that receiver finished decoding */

    return 0;
}

/* This function opens the message queue for communicating with opengl */
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

/* This function opens the message queue */
void openSharedMemory()
{
    key_t key = ftok(".", MEM_SEED);
    if (key == -1) /* generate key */
    {
        perror("RECEIVER: key generation");
        exit(3);
    }
    if ((shmid = shmget(key, 0, 0)) == -1) /* open message queue */
    {
        perror("RECEIVER: shmid");
        exit(3);
    }

    if ((sharedMemory = (struct MEMORY *)shmat(shmid, NULL, 0)) == (struct MEMORY *)-1) /* Attach pointer to shared memory */
    {
        perror("RECEIVER: shmat");
        exit(3);
    }

    /* Initilize the number of columns and rows by the values saved in shared memory */
    numOfColumns = sharedMemory->numOfColumns;
    numOfRows = sharedMemory->numOfRows;

    /* Same process done to open the shared memory for the number of readers*/
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
    if ((readers = (struct NUM_OF_READERS *)shmat(r_shmid, NULL, 0)) == (struct NUM_OF_READERS *)-1)
    {
        perror("RECEIVER: shmat");
        exit(3);
    }
}

/* This function is used to attach spy to read, write and mutex semaphores */
void openSemaphores()
{
    int *semid[] = {&mut_semid, &r_semid, &w_semid};
    int seeds[] = {SEM_MUT_SEED, SEM_R_SEED, SEM_W_SEED};

    key_t key;
    for (int i = 0; i < 3; i++) /* loop all smaphores */
    {
        if ((key = ftok(".", seeds[i])) == -1) /* Generate key */
        {
            perror("RECEIVER: semaphore key generation");
            exit(1);
        }
        if ((*semid[i] = semget(key, numOfColumns, 0)) == -1) /* Attach pointer to shared memory */
        {
            perror("RECEIVER: semget obtaining semaphore");
            exit(2);
        }
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
                if (c == '1' && i + 1 < substr.length() && substr[i + 1] == '0')
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
    ofstream receiverFile;
    receiverFile.open("../outputs/receiver.txt");

    for (int i = 0; i < numOfRows; i++)
    {
        receiverFile << decodedRows[i] << "\n";
        cout << "RECEIVER decoding " << decodedRows[i] << endl;
    }

    receiverFile.close();
}

/* This function adds the needed signal catcher */
void addSignalCatcher()
{
    if (sigset(SIGUSR1, finishSignalCatcher) == SIG_ERR) /* Receiver is interrupted by master spy to be killed */
    {
        perror("SIGUSR1 handler");
        exit(6);
    }
}

/* This function is called when SIGUSR1 is signaled */
void finishSignalCatcher(int signum)
{
    shmdt(sharedMemory);
    exit(signum);
}