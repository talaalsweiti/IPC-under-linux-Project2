#include "local.h"
using namespace std;

void createSharedMemory();
void openSemaphores();
void finishSignalCatcher(int);

struct MEMORY *sharedMemory;
int shmid, r_semid, w_semid;
int numOfColumns;

static struct sembuf acquire = {0, -1, SEM_UNDO},
                     release = {0, 1, SEM_UNDO};

int main()
{
    cout << "***************IM IN HELPER*************" << endl;
    if (sigset(SIGUSR1, finishSignalCatcher) == SIG_ERR) /* child is interrupted by SIGINT */
    {
        perror("SIGUSR1 handler");
        exit(6);
    }
    srand(getpid());
    unsigned col1, col2;
    createSharedMemory();
    openSemaphores();

    while (1)
    {
        col1 = rand() % numOfColumns;
        col2 = rand() % numOfColumns;

        // Block SIGUSR1
        sigset_t signalSet;
        sigemptyset(&signalSet);
        sigaddset(&signalSet, SIGUSR1);
        sigprocmask(SIG_BLOCK, &signalSet, NULL);

        if (col1 > col2)
        {
            swap(col1, col2);
        }
        else if (col1 == col2)
        {
            continue;
        }

        acquire.sem_num = col1;
        // cout << "HELPER ID: " << getpid() << " acquire " << col1 << endl;
        if (semop(w_semid, &acquire, 1) == -1)
        {
            perror("HELPER: semop write sem1");
            exit(3);
        }
        if (semop(r_semid, &acquire, 1) == -1)
        {
            perror("HELPER: semop read sem1");
            exit(3);
        }

        // cout << "HELPER ID: " << getpid() << " acquire " << col2 << endl;
        acquire.sem_num = col2;
        if (semop(w_semid, &acquire, 1) == -1)
        {
            perror("HELPER: semop write sem2");
            exit(3);
        }
        if (semop(r_semid, &acquire, 1) == -1)
        {
            perror("HELPER: semop read sem2");
            exit(3);
        }

        // cout << "HELPER ID : " << getpid() << " is writting" << endl;

        char temp[MAX_STRING_LENGTH];
        strcpy(temp, sharedMemory->data[col1]);
        strcpy(sharedMemory->data[col1], sharedMemory->data[col2]);
        strcpy(sharedMemory->data[col2], temp);
        // cout << "HELPER ID : " << getpid() << " finished writting" << endl;

        release.sem_num = col1;
        // cout << "HELPER ID : " << getpid() << " requst " << col1 << endl;
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

        // cout << "HELPER " << getpid() << " re " << col2 << endl;
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

        sleep(rand() % 5);
    }
    return 0;
}

void createSharedMemory()
{
    key_t key = ftok(".", MEM_SEED);
    if (key == -1)
    {
        perror("HELPER: key generation");
        exit(3);
    }
    if ((shmid = shmget(key, 0, 0)) == -1)
    {
        perror("HELPER: shmid");
        exit(3);
    }
    // Attach the shared memory segment
    if ((sharedMemory = (struct MEMORY *)shmat(shmid, NULL, 0)) == (struct MEMORY *)-1)
    {
        perror("HELPER: shmat");
    }
    numOfColumns = sharedMemory->numOfColumns;
}
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
void finishSignalCatcher(int signum)
{
    shmdt(sharedMemory);
    exit(signum);
}