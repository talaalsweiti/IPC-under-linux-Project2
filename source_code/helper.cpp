#include "local.h"

using namespace std;

/* Add functions' prototypes */
void openSharedMemory();
void openSemaphores();
void finishSignalCatcher(int);

/* Define memory pointer and semaphores variables*/
struct MEMORY *sharedMemory;
int shmid, r_semid, w_semid;
int numOfColumns;

/* define two struct variables, 'acquire' and 'release', which specify the parameters for acquiring and releasing a semaphore respectively */
static struct sembuf acquire = {0, -1, SEM_UNDO},
                     release = {0, 1, SEM_UNDO};

int main()
{
    cout << "***************IM IN HELPER*************" << endl;

    if (sigset(SIGUSR1, finishSignalCatcher) == SIG_ERR) /* Signaled by parent when cleanup is performed */
    {
        perror("SIGUSR1 handler");
        exit(6);
    }

    srand(getpid()); /* Set helper PID as unique seed for rand */

    unsigned col1, col2;

    openSharedMemory();
    openSemaphores();

    while (1) /* keep swapping columns continuously until killed by parent */
    {
        /* select 2 random columns */
        col1 = rand() % numOfColumns;
        col2 = rand() % numOfColumns;

        /*
            blocking SIGUSR1 before aquiring the semaphore so
            the helper wouldn't get killed while having the semaphore
        */
        sigset_t signalSet;
        sigemptyset(&signalSet);
        sigaddset(&signalSet, SIGUSR1);
        sigprocmask(SIG_BLOCK, &signalSet, NULL);

        /* swap if needed to prevent deadlock */
        if (col1 > col2)
        {
            swap(col1, col2);
        }
        else if (col1 == col2) /* remove case of swapping the same column */
        {
            continue;
        }

        /*
            To perform swapping on memory, the helper will be writing.
            To do that, it needs to acquire both the read and the write semaphores
        */

        /* Acquire semaphores for column 1 */
        acquire.sem_num = col1;
        if (semop(w_semid, &acquire, 1) == -1) /* aquire write semaphore for column 1 */
        {
            perror("HELPER: semop write sem1");
            exit(3);
        }
        if (semop(r_semid, &acquire, 1) == -1) /* aquire read semaphore for column 1 */
        {
            perror("HELPER: semop read sem1");
            exit(3);
        }

        /* Acquire semaphores for column 2 */
        acquire.sem_num = col2;
        if (semop(w_semid, &acquire, 1) == -1) /* aquire write semaphore for column 2 */
        {
            perror("HELPER: semop write sem2");
            exit(3);
        }
        if (semop(r_semid, &acquire, 1) == -1) /* aquire read semaphore for column 2 */
        {
            perror("HELPER: semop read sem2");
            exit(3);
        }

        /* Perform swapping between the two columns */
        char temp[MAX_STRING_LENGTH];
        strcpy(temp, sharedMemory->data[col1]);
        strcpy(sharedMemory->data[col1], sharedMemory->data[col2]);
        strcpy(sharedMemory->data[col2], temp);

        /* Release read and write semaphores for columns 1 */
        release.sem_num = col1;
        if (semop(r_semid, &release, 1) == -1)
        {
            perror("HELPER: semop read sem1");
            exit(3);
        }
        if (semop(w_semid, &release, 1) == -1)
        {
            perror("HELPER: semop write sem1");
            exit(3);
        }

        /* Release read and write semaphores for columns 2 */
        release.sem_num = col2;
        if (semop(r_semid, &release, 1) == -1)
        {
            perror("HELPER: semop read sem2");
            exit(3);
        }
        if (semop(w_semid, &release, 1) == -1)
        {
            perror("HELPER: semop write sem2");
            exit(3);
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

        sleep(rand() % 5); /* random sleep before swapping again */
    }
    return 0;
}

/* This function is used to attach the helper to the shared memory and initilize number of columns */
void openSharedMemory()
{
    key_t key = ftok(".", MEM_SEED); /* Get the shared memory key */
    if (key == -1)
    {
        perror("HELPER: key generation");
        exit(3);
    }
    if ((shmid = shmget(key, 0, 0)) == -1) /* Get shared memory with the specified key */
    {
        perror("HELPER: shmid");
        exit(3);
    }
    if ((sharedMemory = (struct MEMORY *)shmat(shmid, NULL, 0)) == (struct MEMORY *)-1) /* Attach to shared memory */
    {
        perror("HELPER: shmat");
    }
    numOfColumns = sharedMemory->numOfColumns; /* Save the number of columns to use */
}

/* This function is used to attach helper to read and write semaphores */
void openSemaphores()
{
    int *semid[] = {&r_semid, &w_semid};
    int seeds[] = {SEM_R_SEED, SEM_W_SEED};

    key_t key;
    for (int i = 0; i < 2; i++)
    {
        if ((key = ftok(".", seeds[i])) == -1)
        {
            perror("HELPER: semaphore key generation");
            exit(1);
        }
        if ((*semid[i] = semget(key, numOfColumns, 0)) == -1)
        {
            perror("HELPER: semget obtaining semaphore");
            exit(2);
        }
    }
}

/* This function is called when helper is interrupted with SIGUSR1 */
void finishSignalCatcher(int signum)
{
    shmdt(sharedMemory); /* Detach shared memory */
    exit(signum);
}