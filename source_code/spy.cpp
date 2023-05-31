/* Spy tries to obtain a column randomly from memory and send it to master spy */
#include "local.h"

using namespace std;

/* Add functions' prototypes */
void openSharedMemory();
void openSemaphores();
void openMessageQueue();
void finishSignalCatcher(int);

int numOfColumns;
static struct MEMORY *sharedMemory;
static struct NUM_OF_READERS *readers; /* Define pointer to the shared memory for the number of readers */
int mid, shmid, r_shmid, mut_semid, r_semid, w_semid;
MESSAGE msg;

/* define two struct variables, 'acquire' and 'release', which specify the parameters for acquiring and releasing a semaphore respectively */
static struct sembuf acquire = {0, -1, SEM_UNDO},
                     release = {0, 1, SEM_UNDO};

int main()
{
    cout << "***************IM IN SPY*************" << endl;

    if (sigset(SIGUSR1, finishSignalCatcher) == SIG_ERR) /* Spy is interrupted by master spy to be killed */
    {
        perror("SIGUSR1 handler");
        exit(6);
    }

    openMessageQueue();
    openSharedMemory();
    openSemaphores();

    srand(getpid()); /* Set spy PID as unique seed for rand */

    unsigned col;

    /* Access a random column and send it to master spy */
    while (1)
    {

        col = rand() % numOfColumns; /* Randomly select a column */

        /*
            blocking SIGUSR1 before aquiring the semaphore so
            the spy wouldn't get killed while having the semaphore
        */
        sigset_t signalSet;
        sigemptyset(&signalSet);
        sigaddset(&signalSet, SIGUSR1);
        sigprocmask(SIG_BLOCK, &signalSet, NULL);

        /* Acquire mutex and write semaphores to increase the number of readers */
        acquire.sem_num = col;
        release.sem_num = col;
        if (semop(w_semid, &acquire, 1) == -1) /* Aquire write sempahore */
        {
            perror("SPY: semop write sem");
            exit(3);
        }
        if (semop(mut_semid, &acquire, 1) == -1) /* Acquire mutex semaphore */
        {
            perror("SPY: semop mut sem");
            exit(3);
        }
        readers->readers[col]++; /* increment the number of readers for a specific column */

        if (semop(mut_semid, &release, 1) == -1) /* release mutex semaphore */
        {
            perror("SPY: semop read sem");
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
                perror("SPY: semop read sem");
                exit(3);
            }
        }

        if (semop(w_semid, &release, 1) == -1) /* Release write semaphore */
        {
            perror("SPY: semop write sem");
            exit(3);
        }

        /* get column from shared memory and save it in buffer */
        memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));
        strcpy(msg.buffer, sharedMemory->data[col]);

        /* send column to master spy using a message queue */
        msg.msg_to = getppid();
        cout << "SPY MESSAGE: " << msg.buffer << endl;
        msgsnd(mid, &msg, strlen(msg.buffer), 0);

        if (semop(mut_semid, &acquire, 1) == -1) /* Acquire mutex semaphore */
        {
            perror("SPY: semop mut sem");
            exit(3);
        }

        readers->readers[col]--; /* Decrement number of readers */

        if (semop(mut_semid, &release, 1) == -1) /* release mutex semaphore */
        {
            perror("SPY: semop mut sem");
            exit(3);
        }

        if (readers->readers[col] == 0) /* if last reader, release read semaphore */
        {
            if (semop(r_semid, &release, 1) == -1)
            {
                perror("SPY: semop read sem");
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
            printf("Handling pending SIGUSR1 signal...\n");
            break;
        }

        sleep(rand() % 8); /* random sleep before getting another column */
    }

    shmdt(sharedMemory); /* detach shared memory */
    return 0;
}

/* This function opens the message queue */
void openMessageQueue()
{
    key_t key;
    if ((key = ftok(".", Q_SEED)) == -1) /* generate key */
    {
        perror("SPY: key generation");
        exit(1);
    }

    if ((mid = msgget(key, 0)) == -1) /* open message queue */
    {
        perror("SPY: Queue openning");
        exit(2);
    }
}

/* This function is used to attach the spy to the shared memory and initilize number of columns */
void openSharedMemory()
{
    key_t key = ftok(".", MEM_SEED);
    if (key == -1) /* Generate key */
    {
        perror("SPY: key generation");
        exit(3);
    }
    if ((shmid = shmget(key, 0, 0)) == -1) /* get the shared memory */
    {
        perror("SPY: shmid");
        exit(3);
    }
    if ((sharedMemory = (struct MEMORY *)shmat(shmid, NULL, 0)) == (struct MEMORY *)-1) /* Attach pointer to shared memory */
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
    for (int i = 0; i < 3; i++) /* loop all smaphores */
    {
        if ((key = ftok(".", seeds[i])) == -1) /* Generate key */
        {
            perror("SPY: semaphore key generation");
            exit(1);
        }
        if ((*semid[i] = semget(key, numOfColumns, 0)) == -1) /* Attach pointer to shared memory */
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