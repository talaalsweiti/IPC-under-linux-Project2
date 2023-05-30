#include "local.h"

using namespace std;

/* Add functions' prototypes */
void openSharedMemory();
void openSemaphores();
void openMessageQueue();
void finishSignalCatcher(int);

int numOfColumns;                      /* Define number of columns to be read from shared memory */
static struct MEMORY *sharedMemory;    /* define a pointer to the shared memory */
static struct NUM_OF_READERS *readers; /* Define pointer to the shared memory for the number of readers */
int mid, shmid, r_shmid, mut_semid, r_semid, w_semid;
MESSAGE msg; /* define variable to hold the message read from message queue */

/* Define memory pointer and semaphores variables*/
static struct sembuf acquire = {0, -1, SEM_UNDO},
                     release = {0, 1, SEM_UNDO};

int main()
{
    cout << "***************IM IN SPY*************" << endl;

    if (sigset(SIGUSR1, finishSignalCatcher) == SIG_ERR) /* Child is interrupted by master spy to be killed */
    {
        perror("SIGUSR1 handler");
        exit(6);
    }

    pid_t ppid = getppid();

    openMessageQueue(); /* Open message queue */
    openSharedMemory(); /* Open shared memory */
    openSemaphores(); /* Open a and attach to semaphores */

    srand(getpid());  /* Set helper PID as unique seed for rand */

    unsigned col;
    // Access and modify the shared matrix, to get all columns
    while (1)
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
            break;
        }

        sleep(rand() % 8);
    }

    shmdt(sharedMemory);
    return 0;
}

/* This function opens the message queue with  */
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

/* This function is used to attach the spy to the shared memory and initilize number of columns */
void openSharedMemory()
{
    key_t key = ftok(".", MEM_SEED);
    if (key == -1) /* get the key */
    {
        perror("SPY: key generation");
        exit(3);
    }
    if ((shmid = shmget(key, 0, 0)) == -1) /* get the shared memory */
    {
        perror("SPY: shmid");
        exit(3);
    }
    if ((sharedMemory = (struct MEMORY *)shmat(shmid, NULL, 0)) == (struct MEMORY *)-1) /* attach the shared memory segment */
    {
        perror("SPY: shmat");
        exit(3);
    }
    numOfColumns = sharedMemory->numOfColumns; /* Initilize the number of columns by the value saved in shared memory */

    /* Same process done to open the shared memory for the number of readers*/
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
    if ((readers = (struct NUM_OF_READERS *)shmat(r_shmid, NULL, 0)) == (struct NUM_OF_READERS *)-1)
    {
        perror("SPY: shmat");
        exit(3);
    }
}

/* This function is used to attach spy to read, write and mutex semaphores */
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

/* This function is called when spy is interrupted with SIGUSR1 */
void finishSignalCatcher(int signum)
{
    shmdt(sharedMemory); /* Detach shared memory */
    exit(signum);
}