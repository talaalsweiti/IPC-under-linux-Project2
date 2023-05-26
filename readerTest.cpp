#include "local.h"

using namespace std;
int numOfColumns;
static struct MEMORY *sharedMemory;
static struct NUM_OF_READERS *readers;
int shmid, r_shmid, mut_semid, r_semid, w_semid;

static struct sembuf acquire = {0, -1, SEM_UNDO},
                     release = {0, 1, SEM_UNDO};

void openSharedMemory();
void openSemaphores();

int main()
{
    openSharedMemory();
    openSemaphores();
    int j = 10;
    srand(getpid());
    // Access and modify the shared matrix
    while (j--)
    {

        for (int i = 0; i < numOfColumns; i++)
        {
            acquire.sem_num = i;
            release.sem_num = i;
            if (semop(w_semid, &acquire, 1) == -1)
            {
                perror("TEST: semop write sem");
                exit(3);
            }
            if (semop(mut_semid, &acquire, 1) == -1)
            {
                perror("TEST: semop mut sem");
                exit(3);
            }
            readers->readers[i]++;

            if (semop(mut_semid, &release, 1) == -1)
            {
                perror("TEST: semop read sem");
                exit(3);
            }
            if (readers->readers[i] == 1)
            {
                if (semop(r_semid, &acquire, 1) == -1)
                {
                    perror("TEST: semop read sem");
                    exit(3);
                }
            }

            if (semop(w_semid, &release, 1) == -1)
            {
                perror("TEST: semop write sem");
                exit(3);
            }
            // perform reading
            cout << sharedMemory->data[i] << endl;
            cout << "    READING COL DONE " << endl;

            // done reading

            if (semop(mut_semid, &acquire, 1) == -1)
            {
                perror("TEST: semop mut sem");
                exit(3);
            }
            readers->readers[i]--;

            if (semop(mut_semid, &release, 1) == -1)
            {
                perror("TEST: semop mut sem");
                exit(3);
            }

            if (readers->readers[i] == 0)
            {
                if (semop(r_semid, &release, 1) == -1)
                {
                    perror("TEST: semop read sem");
                    exit(3);
                }
            }
        }
        // sleep(rand() % 5);
    }

    shmdt(sharedMemory);

    // shmctl(shmid, IPC_RMID, (struct shmid_ds *)0);
    // Other processes can now attach to the same shared memory segment using the same key
    return 0;
}

void openSharedMemory()
{
    key_t key = ftok(".", MEM_SEED);
    if (key == -1)
    {
        perror("TEST: key generation");
        exit(3);
    }
    if ((shmid = shmget(key, 0, 0)) == -1)
    {
        perror("TEST: shmid");
        exit(3);
    }
    // Attach the shared memory segment
    if ((sharedMemory = (struct MEMORY *)shmat(shmid, NULL, 0)) == (struct MEMORY *)-1)
    {
        perror("TEST: shmat");
    }
    numOfColumns = sharedMemory->rows;

    key = ftok(".", MEM_NUM_OF_READERS_SEED);
    if (key == -1)
    {
        perror("TEST: key generation");
        exit(3);
    }
    if ((r_shmid = shmget(key, 0, 0)) == -1)
    {
        perror("TEST: shmid");
        exit(3);
    }
    // Attach the shared memory segment
    if ((readers = (struct NUM_OF_READERS *)shmat(r_shmid, NULL, 0)) == (struct NUM_OF_READERS *)-1)
    {
        perror("TEST: shmat");
    }
    numOfColumns = readers->numOfColumns;
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
            perror("TEST: semaphore key generation");
            exit(1);
        }
        if ((*semid[i] = semget(key, numOfColumns, 0)) == -1)
        {
            perror("TEST: semget obtaining semaphore");
            exit(2);
        }
    }
}