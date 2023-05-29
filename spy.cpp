#include "local.h"

using namespace std;
int numOfColumns, numOfRows;
static struct MEMORY *sharedMemory;
static struct NUM_OF_READERS *readers;
int mid, shmid, r_shmid, mut_semid, r_semid, w_semid;
bool flag = true;
MESSAGE msg;

static struct sembuf acquire = {0, -1, SEM_UNDO},
                     release = {0, 1, SEM_UNDO};

void openSharedMemory();
void openSemaphores();
void openMessageQueue();
void finishSignalCatcher(int);

int main()
{
    cout << "***************IM IN SPY*************" << endl;

    if (sigset(SIGUSR1, finishSignalCatcher) == SIG_ERR) /* child is interrupted by SIGINT */
    {
        perror("SIGUSR1 handler");
        exit(6);
    }

    pid_t ppid = getppid();
    openMessageQueue();
    openSharedMemory();
    openSemaphores();
    srand(getpid());

    unsigned col;
    // Access and modify the shared matrix, to get all columns
    while (flag)
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
            perror("SPY: semop write sem");
            exit(3);
        }
        if (semop(mut_semid, &acquire, 1) == -1)
        {
            perror("SPY: semop mut sem");
            exit(3);
        }
        readers->readers[col]++;

        if (semop(mut_semid, &release, 1) == -1)
        {
            perror("SPY: semop read sem");
            exit(3);
        }
        if (readers->readers[col] == 1)
        {
            if (semop(r_semid, &acquire, 1) == -1)
            {
                perror("SPY: semop read sem");
                exit(3);
            }
        }

        if (semop(w_semid, &release, 1) == -1)
        {
            perror("SPY: semop write sem");
            exit(3);
        }

        memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));
        strcpy(msg.buffer, sharedMemory->data[col]);
        // msg.buffer[MAX_STRING_LENGTH - 1] = '\0';
        msg.msg_to = ppid;
        cout << "SPY MESSAGE: " << msg.buffer << endl;
        msgsnd(mid, &msg, strlen(msg.buffer), 0);
        // cout << "SPY READING DONE " << endl;

        if (semop(mut_semid, &acquire, 1) == -1)
        {
            perror("SPY: semop mut sem");
            exit(3);
        }
        readers->readers[col]--;

        if (semop(mut_semid, &release, 1) == -1)
        {
            perror("SPY: semop mut sem");
            exit(3);
        }

        if (readers->readers[col] == 0)
        {
            if (semop(r_semid, &release, 1) == -1)
            {
                perror("SPY: semop read sem");
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
            // Handle the signal here
            
            break;
        }

        sleep(rand() % 8);
    }

    shmdt(sharedMemory);

    // shmctl(shmid, IPC_RMID, (struct shmid_ds *)0);
    return 0;
}

void openMessageQueue()
{
    key_t key;
    if ((key = ftok(".", Q_SEED)) == -1)
    {
        perror("SPY: key generation");
        exit(1);
    }

    if ((mid = msgget(key, 0)) == -1)
    {
        perror("SPY: Queue openning");
        exit(2);
    }
}

void openSharedMemory()
{
    key_t key = ftok(".", MEM_SEED);
    if (key == -1)
    {
        perror("SPY: key generation");
        exit(3);
    }
    if ((shmid = shmget(key, 0, 0)) == -1)
    {
        perror("SPY: shmid");
        exit(3);
    }
    // Attach the shared memory segment
    if ((sharedMemory = (struct MEMORY *)shmat(shmid, NULL, 0)) == (struct MEMORY *)-1)
    {
        perror("SPY: shmat");
        exit(3);
    }
    numOfColumns = sharedMemory->numOfColumns;
    numOfRows = sharedMemory->numOfRows;

    key = ftok(".", MEM_NUM_OF_READERS_SEED);
    if (key == -1)
    {
        perror("SPY: key generation");
        exit(3);
    }
    if ((r_shmid = shmget(key, 0, 0)) == -1)
    {
        perror("SPY: shmid");
        exit(3);
    }
    // Attach the shared memory segment
    if ((readers = (struct NUM_OF_READERS *)shmat(r_shmid, NULL, 0)) == (struct NUM_OF_READERS *)-1)
    {
        perror("SPY: shmat");
        exit(3);
    }
    // numOfColumns = readers->numOfColumns;
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
            perror("SPY: semaphore key generation");
            exit(1);
        }
        if ((*semid[i] = semget(key, numOfColumns, 0)) == -1)
        {
            perror("SPY: semget obtaining semaphore");
            exit(2);
        }
    }
}

void finishSignalCatcher(int signum)
{
    // cout << "DONE\n";
    shmdt(sharedMemory);
    exit(signum);
}