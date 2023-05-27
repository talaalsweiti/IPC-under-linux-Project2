#include "local.h"
using namespace std;

int NUM_OF_SPIES = 4;
int NUM_OF_HELPERS = 3;

void readFile();
void sendColumnToChildren();
void openSharedMemory();

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
    readFile();
    openSharedMemory();
    sendColumnToChildren();
    return 0;
}

void readFile()
{
    FILE *fin;
    string command = "sed -e \"s/\\s\\ */\\ /g; /^$/d\" sender.txt | fmt -s -w 80 sender.txt > /tmp/sender.txt && mv /tmp/sender.txt sender.txt";
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
}

void sendColumnToChildren()
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

    // char *colNumStr = NULL;

    for (int i = 0; i < numOfColumns; i++)
    {
        // sprintf(colNumStr, "%d", i);
        pid_t pid = createProcesses("./child");
        if (pid == -1)
        {
            exit(1);
        }
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
        if (status != 0)
        { // TODO : check & kill ther children
            exit(1);
        }
    }

    msgctl(mid, IPC_RMID, (struct msqid_ds *)0);
    // ------------------------------------------------------------------------
}

void openSharedMemory()
{
    key_t key;
    if ((key = ftok(".", MEM_SEED)) == -1)
    {
        perror("PARENT: shared memory key generation");
        exit(1);
    }

    // open shared memory
    if ((shmid = shmget(key, 0, 0)) == -1) // TODO: remove if already exist??
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

    shmdt(sharedMemory); // weee

    // // just for testing
    // for (int i = 0; i < sharedMemory->rows; i++)
    // {
    //     strncpy(sharedMemory->data[i], "Tala", MAX_STRING_LENGTH - 1);
    //     sharedMemory->data[i][MAX_STRING_LENGTH - 1] = '\0';
    // }
}