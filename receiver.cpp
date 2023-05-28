#include "local.h"

using namespace std;
int numOfColumns;
static struct MEMORY *sharedMemory;
static struct NUM_OF_READERS *readers;
int shmid, r_shmid, mut_semid, r_semid, w_semid;

static struct sembuf acquire = {0, -1, SEM_UNDO},
                     release = {0, 1, SEM_UNDO};

void createSharedMemory();
void openSemaphores();

int main()
{
    createSharedMemory();
    openSemaphores();
    srand(getpid());
    char cols[numOfColumns][MAX_STRING_LENGTH];
    bool isExist[numOfColumns] = {false};
  
    unsigned col;
    int cntCols = 0;
    // Access and modify the shared matrix, to get all columns
    while (cntCols < numOfColumns)
    {

        col = rand() % numOfColumns;
        cout <<  "IM RANDOM COL: " << col << endl;

        acquire.sem_num = col;
        release.sem_num = col;
        if (semop(w_semid, &acquire, 1) == -1)
        {
            perror("RECEIVER: semop write sem");
            exit(3);
        }
        if (semop(mut_semid, &acquire, 1) == -1)
        {
            perror("RECEIVER: semop mut sem");
            exit(3);
        }
        readers->readers[col]++;

        if (semop(mut_semid, &release, 1) == -1)
        {
            perror("RECEIVER: semop read sem");
            exit(3);
        }
        if (readers->readers[col] == 1)
        {
            if (semop(r_semid, &acquire, 1) == -1)
            {
                perror("RECEIVER: semop read sem");
                exit(3);
            }
        }

        if (semop(w_semid, &release, 1) == -1)
        {
            perror("RECEIVER: semop write sem");
            exit(3);
        }

        stringstream sline(sharedMemory->data[col]);
        if (!sline.good())
        {
            continue;
        }
        string colNumStr;
        getline(sline, colNumStr, ' ');
        int colNum = stoi(colNumStr);
        if (isExist[colNum-1])
        {
            continue;
        }
        cntCols++;
        isExist[colNum-1] = true;

        strncpy(cols[colNum-1], sharedMemory->data[col], MAX_STRING_LENGTH - 1);
        cols[colNum-1][MAX_STRING_LENGTH - 1] = '\0';
        cout << col << " -- " << sharedMemory->data[col] << endl;
        cout << "    READING COL DONE " << endl;

        // done reading

        if (semop(mut_semid, &acquire, 1) == -1)
        {
            perror("TEST: semop mut sem");
            exit(3);
        }
        readers->readers[col]--;

        if (semop(mut_semid, &release, 1) == -1)
        {
            perror("TEST: semop mut sem");
            exit(3);
        }

        if (readers->readers[col] == 0)
        {
            if (semop(r_semid, &release, 1) == -1)
            {
                perror("TEST: semop read sem");
                exit(3);
            }
        }

        sleep(rand() % 3);
    }

    shmdt(sharedMemory);

    // shmctl(shmid, IPC_RMID, (struct shmid_ds *)0);
    // Other processes can now attach to the same shared memory segment using the same key
    return 0;
}

void createSharedMemory()
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
        exit(3);
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