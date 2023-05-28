#include "local.h"
using namespace std;

int NUM_OF_SPIES;
int NUM_OF_HELPERS;
int THRESHOLD;

void readInputVariablesFile();
void createMessageQueue();
void sendColumnToChildren();
void createSharedMemory();
void createSemaphore(key_t, int);
void createSemaphores();
void createReaderSharedVariable();
void cleanup();
void getNumberOfColumns();
vector<vector<string>> tokens;
int numOfColumns = 0, numOfRows;
key_t key;
int mid, n;
MESSAGE msg;

struct MEMORY *sharedMemory;
int shmid, r_shmid, semid[3];
pid_t *helpers, *spies;
pid_t sender, reciever, spy;
pid_t pid = getpid();

union semun arg;

int main(int argc, char *argv[])
{
    readInputVariablesFile();
    int status;
    createMessageQueue();
    sender = createProcesses("./sender");
    waitpid(sender, &status, 0);
    // TODO: check if correct
    if (status != 0)
    {
        cout << "sender failed" << endl;
        exit(1);
    }
    getNumberOfColumns();
    createReaderSharedVariable();
    createSemaphores();

    // create helpers, assume 2 helpers
    helpers = new pid_t[NUM_OF_HELPERS]; // TODO: free?

    // sleep(2);
    for (int i = 0; i < NUM_OF_HELPERS; i++)
    {
        helpers[i] = createProcesses("./helper");
    }

    reciever = createProcesses("./receiver");

    // TODO: to reomve
    waitpid(reciever, &status, 0);
    if (status != 0)
    {
        cout << "recevier failed" << endl;
        exit(2);
    }
    for (int i = 0; i < NUM_OF_HELPERS; i++)
    {
        waitpid(helpers[i], NULL, 0);
    }

    sleep(5);
    cleanup();
    
    return 0;
}

void readInputVariablesFile()
{
    ifstream inputVariableFile("inputVariables.txt");
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
        if(variableName == "NUM_OF_HELPERS"){
            NUM_OF_HELPERS = stoi(value);
        }
        else if(variableName == "NUM_OF_SPIES"){
            NUM_OF_SPIES = stoi(value);
        }
        else if(variableName == "THRESHOLD"){
            THRESHOLD = stoi(value);
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
        exit(1);
    }

    // create shared memory
    if ((shmid = shmget(key, size, IPC_CREAT | 0666)) == -1) // TODO: remove if already exist??
    {
        perror("shmget -- parent -- create");
        exit(1);
    }

    // attach shared memory
    if ((sharedMemory = (struct MEMORY *)shmat(shmid, NULL, 0)) == (struct MEMORY *)-1)
    {
        perror("shmptr -- parent -- attach");
        exit(1);
    }

    sharedMemory->numOfColumns = numOfColumns;

    shmdt(sharedMemory); // weee

    // // just for testing
    // for (int i = 0; i < sharedMemory->numOfColumns; i++)
    // {
    //     strncpy(sharedMemory->data[i], "Tala", MAX_STRING_LENGTH - 1);
    //     sharedMemory->data[i][MAX_STRING_LENGTH - 1] = '\0';
    // }
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

    // int sem_value;
    // for (int j = 0; j < numOfColumns; j++)
    // { /* display contents */
    //     if ((sem_value = semctl(semid[i], j, GETVAL, 0)) == -1)
    //     {
    //         perror("semctl: GETVAL");
    //         exit(4);
    //     }

    //     printf("%d Semaphore%d: %d has value of %d\n", semid[i], i, j, sem_value);
    // }
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

    if (shmctl(shmid, IPC_RMID, (struct shmid_ds *)0) == -1)
    {
        perror("shmctl: IPC_RMID"); /* remove semaphore */
        // exit(5);
    }
    if (shmctl(r_shmid, IPC_RMID, (struct shmid_ds *)0) == -1)
    {
        perror("shmctl: IPC_RMID"); /* remove semaphore */
        // exit(5);
    }
    msgctl(mid, IPC_RMID, (struct msqid_ds *)0);

    for (int i = 0; i < 3; i++)
    {
        if (semctl(semid[i], 0, IPC_RMID, 0) == -1)
        {
            perror("semctl: IPC_RMID"); /* remove semaphore */
            // exit(5);
        }
    }
}

void getNumberOfColumns()
{
    key_t key;
    if ((key = ftok(".", MEM_SEED)) == -1)
    {
        perror("PARENT: shared memory key generation");
        exit(1);
    }

    if ((shmid = shmget(key, 0, 0)) == -1) // TODO: remove if already exist??
    {
        perror("shmget -- parent -- create");
        exit(1);
    }

    // Attach the shared memory segment
    if ((sharedMemory = (struct MEMORY *)shmat(shmid, NULL, 0)) == (struct MEMORY *)-1)
    {
        perror("RECEIVER: shmat");
        exit(3);
    }
    numOfColumns = sharedMemory->numOfColumns;
    shmdt(sharedMemory);
}

void createMessageQueue()
{
    if ((key = ftok(".", Q_SEED)) == -1)
    {
        perror("Parent: key generation");
        exit(3);
    }

    if ((mid = msgget(key, IPC_CREAT | 0660)) == -1) // TODO: check for IPC_CREAT flag *********
    {
        perror("Queue creation");
        exit(4);
    }
}