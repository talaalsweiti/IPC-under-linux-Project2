#include "local.h"
using namespace std;

int NUM_OF_SPIES;
int NUM_OF_HELPERS;
int THRESHOLD;
int receiverFailedDecoding = 0;
int receiverSuccessfulDecoding = 0;

bool flg = true;

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
vector<vector<string>> tokens;
int numOfColumns = 0, numOfRows = 0;
key_t key;
int mid, n;
MESSAGE msg;

struct MEMORY *sharedMemory;
int shmid, r_shmid, semid[3];
pid_t *helpers, *spies;
pid_t sender, reciever, masterSpy, opengl;
pid_t pid = getpid();

union semun arg;

int main(int argc, char *argv[])
{
    cout << "***************IM IN PARENT*************" << endl;

    addSignalCatchers();
    readInputVariablesFile();
    int status;
    createMessageQueue();
    sender = createProcesses("./sender");
    waitpid(sender, &status, 0);
    // TODO: check if correct
    if (status != 0)
    {
        cout << "sender failed" << endl;
        cleanup();
        exit(1);
    }
    opengl = createProcesses("./opengl");

    memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));
    strcpy(msg.buffer, to_string(NUM_OF_HELPERS).c_str());
    msg.msg_to = 6;
    msgsnd(mid, &msg, strlen(msg.buffer), 0);
    // sleep(3);
    memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));
    strcpy(msg.buffer, to_string(NUM_OF_SPIES).c_str());
    msg.msg_to = 6;
    msgsnd(mid, &msg, strlen(msg.buffer), 0);

    memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));
    strcpy(msg.buffer, to_string(THRESHOLD).c_str());
    msg.msg_to = 6;
    msgsnd(mid, &msg, strlen(msg.buffer), 0);

    getDimensions();
    createReaderSharedVariable();
    createSemaphores();

    // create helpers, assume 2 helpers
    helpers = new pid_t[NUM_OF_HELPERS]; // TODO: free?

    sleep(2);
    for (int i = 0; i < NUM_OF_HELPERS; i++)
    {
        helpers[i] = createProcesses("./helper");
    }
    char arg1[10], arg2[10], arg3[10]; // TODO
    sprintf(arg1, "%d", NUM_OF_SPIES);
    sprintf(arg2, "%d", numOfColumns);
    sprintf(arg3, "%d", numOfRows);
    int round = 1;
    while (receiverFailedDecoding < THRESHOLD && receiverSuccessfulDecoding < THRESHOLD)
    {
        memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));
        strcpy(msg.buffer, to_string(round).c_str());
        msg.msg_to = 3;
        msgsnd(mid, &msg, strlen(msg.buffer), 0);

        cout << "STARTING" << endl;
        reciever = createProcesses("./receiver");
        masterSpy = createProcesses("./masterSpy", arg1, arg2, arg3);
        while (flg)
        {
            pause();
        }
        flg = true;
        sleep(3);
        round++;
    }

    // sleep(5);
    cleanup();

    return 0;
}

void readInputVariablesFile()
{
    ifstream inputVariableFile("../inputs/inputVariables.txt");
    if (!inputVariableFile.good())
    {
        perror("Open inputVariables.txt");
        exit(2);
    }

    string line;
    while (getline(inputVariableFile, line))
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

void createSharedMemory()
{

    size_t size = sizeof(struct MEMORY) + MAX_STRING_LENGTH * numOfColumns * sizeof(char);
    key_t key;
    if ((key = ftok(".", MEM_SEED)) == -1)
    {
        perror("PARENT: shared memory key generation");
        cleanup();
        exit(1);
    }

    // create shared memory
    if ((shmid = shmget(key, size, IPC_CREAT | 0666)) == -1) // TODO: remove if already exist??
    {
        perror("shmget -- parent -- create");
        cleanup();
        exit(1);
    }

    // attach shared memory
    if ((sharedMemory = (struct MEMORY *)shmat(shmid, NULL, 0)) == (struct MEMORY *)-1)
    {
        perror("shmptr -- parent -- attach");
        cleanup();
        exit(1);
    }

    sharedMemory->numOfColumns = numOfColumns;

    shmdt(sharedMemory);
}

void createReaderSharedVariable()
{
    size_t size = sizeof(struct NUM_OF_READERS) + numOfColumns * sizeof(unsigned);
    key_t key;
    if ((key = ftok(".", MEM_NUM_OF_READERS_SEED)) == -1)
    {
        perror("PARENT_R: shared memory key generation");
        cleanup();
        exit(1);
    }

    // create shared memory
    if ((r_shmid = shmget(key, size, IPC_CREAT | 0666)) == -1) // TODO: remove if already exist??
    {
        perror("PARENT_R: shmget -- parent -- create");
        cleanup();
        exit(1);
    }

    struct NUM_OF_READERS *numOfReadersShmem;

    // attach shared memory
    if ((numOfReadersShmem = (struct NUM_OF_READERS *)shmat(r_shmid, NULL, 0)) == (struct NUM_OF_READERS *)-1)
    {
        perror("shmptr -- parent -- attach");
        cleanup();
        exit(1);
    }
    numOfReadersShmem->numOfColumns = numOfColumns;
    memset(numOfReadersShmem->readers, 0, numOfColumns * sizeof(unsigned));
    shmdt(numOfReadersShmem);
}

void createSemaphore(key_t key, int i)
{
    // create sem
    if ((semid[i] = semget(key, numOfColumns, IPC_CREAT | 0666)) == -1) // TODO: what if already exist?
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
    // memset(start_val, 0x01, sizeof(start_val));
    arg.array = start_val;
    if (semctl(semid[i], 0, SETALL, arg) == -1)
    {
        perror("semctl -- parent -- initialization");
        cleanup();
        exit(3);
    }
}

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

void cleanup()
{
    for (int i = 0; i < NUM_OF_HELPERS; i++)
    {
        kill(helpers[i], SIGUSR1);
    }
    kill(reciever, SIGUSR1);
    kill(masterSpy, SIGUSR2);
    delete[] helpers;
    if (shmctl(shmid, IPC_RMID, (struct shmid_ds *)0) == -1)
    {
        perror("shmctl: IPC_RMID"); /* remove semaphore */
    }
    if (shmctl(r_shmid, IPC_RMID, (struct shmid_ds *)0) == -1)
    {
        perror("shmctl: IPC_RMID"); /* remove semaphore */
    }
    msgctl(mid, IPC_RMID, (struct msqid_ds *)0);

    for (int i = 0; i < 3; i++)
    {
        if (semctl(semid[i], 0, IPC_RMID, 0) == -1)
        {
            perror("semctl: IPC_RMID"); /* remove semaphore */
        }
    }
}

void getDimensions()
{
    key_t key;
    if ((key = ftok(".", MEM_SEED)) == -1)
    {
        perror("PARENT: shared memory key generation");
        cleanup();
        exit(1);
    }

    if ((shmid = shmget(key, 0, 0)) == -1) // TODO: remove if already exist??
    {
        perror("shmget -- parent -- create");
        cleanup();
        exit(1);
    }

    // Attach the shared memory segment
    if ((sharedMemory = (struct MEMORY *)shmat(shmid, NULL, 0)) == (struct MEMORY *)-1)
    {
        perror("PARENT: shmat");
        cleanup();
        exit(3);
    }
    numOfColumns = sharedMemory->numOfColumns;
    numOfRows = sharedMemory->numOfRows;
    shmdt(sharedMemory);
}

void createMessageQueue()
{
    if ((key = ftok(".", Q_SEED)) == -1)
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

void addSignalCatchers()
{
    if (sigset(SIGUSR1, masterSpySignalCatcher) == SIG_ERR)
    {
        perror("SIGUSR1 handler");
        cleanup();
        exit(4);
    }

    if (sigset(SIGUSR2, receiverSignalCatcher) == SIG_ERR)
    {
        perror("SIGUSR2 handler");
        cleanup();
        exit(4);
    }
    if (sigset(SIGINT, processDeadSignalCatcher) == SIG_ERR)
    {
        perror("SIGINT handler");
        cleanup();
        exit(6);
    }
}

void masterSpySignalCatcher(int signum)
{
    // send signal to reciver to stop
    cout << "Master wins, kill reciever" << endl;
    // bool flgg = isCorrectFile("spy.txt");
    // cout << flgg << endl;
    if (isCorrectFile("../outputs/spy.txt"))
    {
        receiverFailedDecoding++;
        memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));
        msg.msg_to = 5;
        strcpy(msg.buffer, to_string(receiverFailedDecoding).c_str());
        msgsnd(mid, &msg, strlen(msg.buffer), 0);
    }
    kill(reciever, SIGUSR1);
    flg = false;
}

void receiverSignalCatcher(int signum)
{
    // send signal to master spy to stop
    cout << "Receiver wins, kill master" << endl;
    // bool flgg = isCorrectFile("receiver.txt");
    // cout << flgg << endl;
    if (isCorrectFile("../outputs/receiver.txt"))
    {
        receiverSuccessfulDecoding++;
        memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));
        msg.msg_to = 4;
        strcpy(msg.buffer, to_string(receiverSuccessfulDecoding).c_str());
        msgsnd(mid, &msg, strlen(msg.buffer), 0);
    }
    kill(masterSpy, SIGUSR2);
    flg = false;
}
void processDeadSignalCatcher(int signum)
{
    cleanup();
    exit(signum);
}

bool isCorrectFile(string file)
{
    FILE *fp;
    char buffer[128];
    string command = "cmp ../inputs/sender.txt " + file;
    fp = popen(command.c_str(), "r");
    if (fp == NULL)
    {
        printf("Failed to run cmp command\n");
        cleanup();
        exit(1);
    }

    bool ans = fgets(buffer, sizeof(buffer), fp) == NULL;

    if (pclose(fp) == -1)
    {
        printf("Error in closing cmp command\n");
        cleanup();
        exit(1);
    }
    return ans;
}