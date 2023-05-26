#include "local.h"
using namespace std;

void readFile();
pid_t createProcesses(char *);
void sendColumnToChildren();
void createSharedMemory();
void createSemaphore(key_t, int);
void createSemaphores();
void createReaderSharedVariable();
void cleanup();

vector<vector<string>> tokens;
int numOfColumns = 0, numOfRows;
key_t key;
int mid, n;
MESSAGE msg;

struct MEMORY *sharedMemory;
int shmid, r_shmid, semid[3];
pid_t *helpers;
pid_t pid = getpid();

union semun arg;

int main(int argc, char *argv[])
{
    readFile();
    createSharedMemory();
    sendColumnToChildren();
    createReaderSharedVariable();
    createSemaphores();

    // create helpers, assume 2 helpers
    helpers = new pid_t[2];

    sleep(2);
    for (int i = 0; i < 2; i++)
    {
        helpers[i] = createProcesses("./helper");
    }

    for (int i = 0; i < 2; i++)
    {
        waitpid(helpers[i], NULL, 0);
    }

    sleep(5);

    cleanup();
    return 0;
}

void readFile()
{
    FILE *fin;
    string command = "sed -i -e \"s/\\s\\ */\\ /g; /^$/d\" sender.txt";
    if ((fin = popen(command.c_str(), "r")) == NULL)
    {
        perror("Preproccessing sender.txt");
        exit(1);
    }
    if (pclose(fin) == -1)
    {
        perror("pclose");
        exit(2);
    }

    ifstream senderFile("sender.txt");
    if (!senderFile.good())
    {
        perror("Open range.txt");
        exit(2);
    }

    string line;
    while (getline(senderFile, line))
    {
        stringstream sline(line);
        vector<string> words;
        while (sline.good())
        {
            string substr;
            getline(sline, substr, ' ');
            words.push_back(substr);
        }
        numOfColumns = max(numOfColumns, (int)words.size());
        tokens.push_back(words);
    }
    for (unsigned i = 0; i < tokens.size(); i++)
    {
        while ((int)tokens[i].size() < numOfColumns)
        {
            tokens[i].push_back("alright");
        }
    }

    // numOfRows = tokens.size();

    // vector<vector<string> > columns(tokens[0].size(), vector<string>(tokens.size()));
    // for (int i = 0; i < tokens[0].size(); i++)
    //     for (int j = 0; j < data.size(); j++) {
    //         columns[i][j] = tokens[j][i];
    //     }
}

void sendColumnToChildren()
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

    // char *colNumStr = NULL;

    for (int i = 0; i < numOfColumns; i++)
    {
        // sprintf(colNumStr, "%d", i);
        pid_t pid = createProcesses("./child");
        msg.msg_to = pid;
        string column = to_string(i + 1); // one indexed column number

        unsigned j;
        for (j = 0; j < tokens.size(); j++)
        {
            column += " " + tokens[j][i];
        }

        strcpy(msg.buffer, column.c_str());
        cout << "PARENT: " << msg.buffer << endl;

        msgsnd(mid, &msg, strlen(msg.buffer), 0);
    }

    // TODO:: this code must be removed, and instead, children should inform the parent when they finish, and then the parent should remove the queue
    for (int i = 0; i < numOfColumns; i++)
    {
        int status;
        wait(&status);
    }

    msgctl(mid, IPC_RMID, (struct msqid_ds *)0);
    // ------------------------------------------------------------------------
}

pid_t createProcesses(char *file)
{
    pid_t pid = fork(); /* fork an opengl child process */
    switch (pid)
    {
    case -1: /* error performing fork */
        perror("Fork");
        cleanup();
        exit(7);

    case 0:                               /* In the child */
        execlp(file, file, (char *)NULL); /* execute passed file */
        perror("exec failure ");
        cleanup();
        exit(8);
        break;

    default:        /* In the parent */
        return pid; /* save the PID of the child */
    }
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

    sharedMemory->rows = numOfColumns;

    shmdt(sharedMemory);

    // // just for testing
    // for (int i = 0; i < sharedMemory->rows; i++)
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
    if ((numOfReadersShmem = (struct NUM_OF_READERS *)shmat(shmid, NULL, 0)) == (struct NUM_OF_READERS *)-1)
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

    for (int i = 0; i < 3; i++)
    {
        if (semctl(semid[i], 0, IPC_RMID, 0) == -1)
        {
            perror("semctl: IPC_RMID"); /* remove semaphore */
            // exit(5);
        }
    }
}