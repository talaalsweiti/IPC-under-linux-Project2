/*
    The Parent class is responsible of creating all children, as well as creating shared memories and semaphores.
    It is also responsible for managing the rounds.
*/

#include "local.h"

using namespace std;

/* Define the variables to be read from inputVariables.txt */
int NUM_OF_SPIES;
int NUM_OF_HELPERS;
int THRESHOLD;

bool neitherFinished = true;

/* Add functions' prototypes */
void readInputVariablesFile();
void createMessageQueue();
void sendColumnToChildren();
void createSharedMemory();
void createSemaphore(key_t, int);
void createSemaphores();
void createReaderSharedVariable();
void cleanup();
void getDimensions();
void masterSpySignalCatcher(int);
void receiverSignalCatcher(int);
void addSignalCatchers();
void processDeadSignalCatcher(int);
bool isCorrectFile(string);

/* Define variables to keep track of the won and lost rounds */
int receiverFailedDecoding = 0;
int receiverSuccessfulDecoding = 0;

int numOfColumns = 0, numOfRows = 0;
key_t key;
int mid, n;
MESSAGE msg;

/* Define memory pointer and semaphores variables*/
struct MEMORY *sharedMemory;
int shmid, r_shmid, semid[3];

/* define pid_t variables to hold the pids of all children and grandchildren */
pid_t *helpers, *spies;
pid_t sender, reciever, masterSpy, opengl;
pid_t pid = getpid();

union semun arg;

int main(int argc, char *argv[])
{
    cout << "***************IM IN PARENT*************" << endl;
    int status;

    addSignalCatchers();
    readInputVariablesFile();
    createMessageQueue();

    /* Create sender process and wait for it to finish executing */
    sender = createProcesses("./sender");
    waitpid(sender, &status, 0);

    if (status != 0)
    {
        cout << "sender failed" << endl;
        cleanup();
        exit(1);
    }

    /* Create opengl file that hold the UI code */
    opengl = createProcesses("./opengl");

    /* Send number of helpers to opengl */
    memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));
    strcpy(msg.buffer, to_string(NUM_OF_HELPERS).c_str());
    msg.msg_to = 6;
    msgsnd(mid, &msg, strlen(msg.buffer), 0);

    /* Send number of spies to opengl */
    memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));
    strcpy(msg.buffer, to_string(NUM_OF_SPIES).c_str());
    msg.msg_to = 6;
    msgsnd(mid, &msg, strlen(msg.buffer), 0);

    /* Send threshold to opengl */
    memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));
    strcpy(msg.buffer, to_string(THRESHOLD).c_str());
    msg.msg_to = 6;
    msgsnd(mid, &msg, strlen(msg.buffer), 0);

    getDimensions(); /* get numOfColumns and numOfRows */
    createReaderSharedVariable();
    createSemaphores();

    // create helpers, assume 2 helpers
    helpers = new pid_t[NUM_OF_HELPERS];

    sleep(2);

    /* Create helper processes */
    for (int i = 0; i < NUM_OF_HELPERS; i++)
    {
        helpers[i] = createProcesses("./helper");
    }

    /* setup args to be passed to master spy process */
    char arg1[10], arg2[10], arg3[10];
    sprintf(arg1, "%d", NUM_OF_SPIES);
    sprintf(arg2, "%d", numOfColumns);
    sprintf(arg3, "%d", numOfRows);

    int round = 1; /* Define round number */

    /* While neither the receiver nor the master spy reached the threshold for winning */
    while (receiverFailedDecoding < THRESHOLD && receiverSuccessfulDecoding < THRESHOLD)
    {
        /* pass round number to opensgl process through a message queue with ID 3 */
        memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));
        strcpy(msg.buffer, to_string(round).c_str());
        msg.msg_to = 3;
        msgsnd(mid, &msg, strlen(msg.buffer), 0);

        cout << "STARTING" << endl;

        /* Create the receiver and the master spy processes */
        reciever = createProcesses("./receiver");
        masterSpy = createProcesses("./masterSpy", arg1, arg2, arg3);

        /* While round is still in progress */
        while (neitherFinished)
        {
            pause();
        }
        neitherFinished = true; /* reset flag */

        sleep(3);

        round++;
    }

    cleanup(); /* Finished rounds, cleanup before exiting */

    return 0;
}

/* This function reads the inputVariables file to get the user-defined variables */
void readInputVariablesFile()
{
    ifstream inputVariableFile("../inputs/inputVariables.txt");
    if (!inputVariableFile.good())
    {
        perror("Open inputVariables.txt");
        exit(2);
    }

    string line;
    while (getline(inputVariableFile, line)) /* Read values and assign to suitable variable */
    {
        stringstream sline(line);
        string variableName;
        getline(sline, variableName, ' ');
        string value;
        getline(sline, value, ' ');
        if (variableName == "NUM_OF_HELPERS")
        {
            NUM_OF_HELPERS = min(stoi(value), 40);
        }
        else if (variableName == "NUM_OF_SPIES")
        {
            NUM_OF_SPIES = min(stoi(value), 40);
        }
        else if (variableName == "THRESHOLD")
        {
            THRESHOLD = min(stoi(value), 40);
        }
    }
    inputVariableFile.close();
}

/* This function connects to the shared memory and  */
void getDimensions()
{
    key_t key;
    if ((key = ftok(".", MEM_SEED)) == -1) /* Generate shared memory key */
    {
        perror("PARENT: shared memory key generation");
        cleanup();
        exit(1);
    }

    if ((shmid = shmget(key, 0, 0)) == -1) /* get shared memory from key */
    {
        perror("shmget -- parent -- create");
        cleanup();
        exit(1);
    }

    if ((sharedMemory = (struct MEMORY *)shmat(shmid, NULL, 0)) == (struct MEMORY *)-1) /* attach pointer to shared memory */
    {
        perror("PARENT: shmat");
        cleanup();
        exit(3);
    }

    /* Read variables saved by the sender from shared memory */
    numOfColumns = sharedMemory->numOfColumns;
    numOfRows = sharedMemory->numOfRows;

    shmdt(sharedMemory); /* Detach from memory */
}

/* Create a shared memory for readers */
void createReaderSharedVariable()
{
    /* Initialize size of shared memory*/
    size_t size = sizeof(struct NUM_OF_READERS) + numOfColumns * sizeof(unsigned); // TODO: max numOfColumns?
    key_t key;
    if ((key = ftok(".", MEM_NUM_OF_READERS_SEED)) == -1) /* Generate key */
    {
        perror("PARENT_R: shared memory key generation");
        cleanup();
        exit(1);
    }

    // TODO: remove if already exist??
    if ((r_shmid = shmget(key, size, IPC_CREAT | 0666)) == -1) /* create shared memory */
    {
        perror("PARENT_R: shmget -- parent -- create");
        cleanup();
        exit(1);
    }

    struct NUM_OF_READERS *numOfReadersShmem;

    if ((numOfReadersShmem = (struct NUM_OF_READERS *)shmat(r_shmid, NULL, 0)) == (struct NUM_OF_READERS *)-1) /* Attach pointer to readers shared memory */
    {
        perror("shmptr -- parent -- attach");
        cleanup();
        exit(1);
    }
    numOfReadersShmem->numOfColumns = numOfColumns;
    memset(numOfReadersShmem->readers, 0, numOfColumns * sizeof(unsigned)); /* initialize readers number in array */

    shmdt(numOfReadersShmem);
}

/* This function takes a key and creates a sempahore with the given key */
void createSemaphore(key_t key, int i)
{
    // TODO: what if already exist?
    if ((semid[i] = semget(key, numOfColumns, IPC_CREAT | 0666)) == -1) /* Create semaphore */
    {
        perror("semget -- parent -- create");
        cleanup();
        exit(2);
    }

    ushort start_val[numOfColumns]; // TODO: change
    for (int k = 0; k < numOfColumns; k++)
    {
        start_val[k] = 1;
    }

    arg.array = start_val;
    if (semctl(semid[i], 0, SETALL, arg) == -1) /* Set initial state of semaphore */
    {
        perror("semctl -- parent -- initialization");
        cleanup();
        exit(3);
    }
}

/* TODO-Shahd
    This function calls for creating 3 semaphores.
    Mutex semaphore -> manage accessing the number of reader
    Read semaphore -> manage accessing shared memory for reading
    Write semaphore -> manage accessing shared memory for writing
*/
void createSemaphores()
{
    int seeds[] = {SEM_MUT_SEED, SEM_R_SEED, SEM_W_SEED};
    for (int i = 0; i < 3; i++)
    {
        key_t key;
        if ((key = ftok(".", seeds[i])) == -1)
        {
            perror("PARENT: semaphore key generation");
            cleanup();
            exit(1);
        }
        createSemaphore(key, i);
    }
}

/* This function create a message queue */
void createMessageQueue()
{
    if ((key = ftok(".", Q_SEED)) == -1) /* generate key for message queue */
    {
        perror("Parent: key generation");
        cleanup();
        exit(3);
    }

    if ((mid = msgget(key, IPC_CREAT | 0660)) == -1) // TODO: check for IPC_CREAT flag *********
    {
        perror("Queue creation");
        cleanup();
        exit(4);
    }
}

/* This function adds the needed signal catchers */
void addSignalCatchers()
{
    if (sigset(SIGUSR1, masterSpySignalCatcher) == SIG_ERR) /* Sent when master wins the round */
    {
        perror("SIGUSR1 handler");
        cleanup();
        exit(4);
    }

    if (sigset(SIGUSR2, receiverSignalCatcher) == SIG_ERR) /* Sent when receiver wins the round */
    {
        perror("SIGUSR2 handler");
        cleanup();
        exit(4);
    }
    if (sigset(SIGINT, processDeadSignalCatcher) == SIG_ERR) /* Handle exiting from program by ^C (using SIGINT) */
    {
        perror("SIGINT handler");
        cleanup();
        exit(6);
    }
}

/* This function is called when parent is interrupted by SIGUSR1 telling that the master spy retrieved all columns */
void masterSpySignalCatcher(int signum)
{
    cout << "Master wins, kill reciever" << endl;
    if (isCorrectFile("../outputs/spy.txt")) /* if decoded file is correct */
    {
        /* increase score for master spy and send it to opengl */
        receiverFailedDecoding++;
        memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));
        msg.msg_to = 5;
        strcpy(msg.buffer, to_string(receiverFailedDecoding).c_str());
        msgsnd(mid, &msg, strlen(msg.buffer), 0);
    }

    kill(reciever, SIGUSR1); /* signal to receiver to stop */
    neitherFinished = false; /* set flag that wakes the parent */
}

/* This function is called when parent is interrupted by SIGUSR2 telling that the receiver retrieved all columns */
void receiverSignalCatcher(int signum)
{
    cout << "Receiver wins, kill master" << endl;
    if (isCorrectFile("../outputs/receiver.txt")) /* if decoded file is correct */
    {
        /* increase score for receiver and send it to opengl */
        receiverSuccessfulDecoding++;
        memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));
        msg.msg_to = 4;
        strcpy(msg.buffer, to_string(receiverSuccessfulDecoding).c_str());
        msgsnd(mid, &msg, strlen(msg.buffer), 0);
    }
    kill(masterSpy, SIGUSR2); /* signal to master spy to stop */
    neitherFinished = false; /* set flag that wakes the parent */
}

/* This function is called when the program is interrupted with SIGINT */
void processDeadSignalCatcher(int signum)
{
    cleanup();
    exit(signum);
}

/* This function checks if the decoded file matches the input file */
bool isCorrectFile(string file)
{
    FILE *fp;
    char buffer[128];
    string command = "cmp ../inputs/sender.txt " + file; /* command to compare both files */
    fp = popen(command.c_str(), "r"); /* execute command */
    if (fp == NULL)
    {
        printf("Failed to run cmp command\n");
        cleanup();
        exit(1);
    }

    /* fgets returns null if the two files are equal */
    bool ans = fgets(buffer, sizeof(buffer), fp) == NULL;

    if (pclose(fp) == -1) /* close file */
    {
        printf("Error in closing cmp command\n");
        cleanup();
        exit(1);
    }
    
    return ans;
}

/* This functions frees all used resources in the program before exiting */
void cleanup()
{
    /* Kill children processes */
    for (int i = 0; i < NUM_OF_HELPERS; i++)
    {
        kill(helpers[i], SIGUSR1);
    }
    kill(reciever, SIGUSR1);
    kill(masterSpy, SIGUSR2);

    delete[] helpers; /* free helpers array */

    /* delete shared memories */
    if (shmctl(shmid, IPC_RMID, (struct shmid_ds *)0) == -1)
    {
        perror("shmctl: IPC_RMID"); /* remove semaphore */
    }
    if (shmctl(r_shmid, IPC_RMID, (struct shmid_ds *)0) == -1)
    {
        perror("shmctl: IPC_RMID"); /* remove semaphore */
    }
    msgctl(mid, IPC_RMID, (struct msqid_ds *)0); /* delete message queue */

    /* delete semaphores */
    for (int i = 0; i < 3; i++) 
    {
        if (semctl(semid[i], 0, IPC_RMID, 0) == -1)
        {
            perror("semctl: IPC_RMID"); /* remove semaphore */
        }
    }
}