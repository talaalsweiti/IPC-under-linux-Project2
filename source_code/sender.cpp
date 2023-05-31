/*
    Sender is responsible for reading the input file,
    and splitting it to columns for children to encode
*/

#include "local.h"

using namespace std;

/* Define functions prototypes */
void readFile();
void sendColumnToChildren();
void createSharedMemory();

/* Define variables needed */
vector<vector<string>> tokens;
int numOfColumns = 0, numOfRows = 0;
int mid;
key_t key;
MESSAGE msg;
struct MEMORY *sharedMemory;
int shmid;

union semun arg;

int main(int argc, char *argv[])
{
    cout << "***************IM IN SENDER*************" << endl;

    readFile();
    createSharedMemory();
    sendColumnToChildren();

    return 0;
}

/* This function reads the input file and fills it in a two dimensional array */
void readFile()
{
    /*
        First, execute a command that removes extra spaces from the file (replace 2 or more executive spaces with one space),
        and format the file as lines instead of a single line per paragraph
    */
    FILE *fin;
    string command = "sed -e \"s/\\s\\ */\\ /g; /^$/d\" ../inputs/sender.txt | fmt -s -w 80 ../inputs/sender.txt > /tmp/sender.txt && mv /tmp/sender.txt ../inputs/sender.txt";
    if ((fin = popen(command.c_str(), "r")) == NULL) /* try executing the command */
    {
        perror("Preproccessing sender.txt");
        exit(1);
    }
    if (pclose(fin) == -1) /* close file */
    {
        perror("pclose");
        exit(2);
    }

    /* open sender.txt file for reading */
    ifstream senderFile("../inputs/sender.txt");
    if (!senderFile.good())
    {
        perror("Open sender.txt");
        exit(2);
    }

    /* read sentences and fill them in the two dimensional array */
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
        numOfColumns = max(numOfColumns, (int)words.size()); /* get the maximum number of columns */
        tokens.push_back(words);
    }

    /* fill the empty spots with "alright" word */
    for (unsigned i = 0; i < tokens.size(); i++)
    {
        while ((int)tokens[i].size() < numOfColumns)
        {
            tokens[i].push_back("alright");
        }
    }
    numOfRows = tokens.size(); /* get the rows size */
}

/* This function creates the shared memory used to store the columns */
void createSharedMemory()
{
    size_t size = sizeof(struct MEMORY) + MAX_STRING_LENGTH * numOfColumns * sizeof(char); /* initialize size of shared memory */
    key_t key;
    if ((key = ftok(".", MEM_SEED)) == -1) /* generate shared memory key */
    {
        perror("SENDER: shared memory key generation");
        exit(1);
    }

    // TODO: remove if already exist??
    if ((shmid = shmget(key, size, IPC_CREAT | 0666)) == -1) /* Create shared memory using key */
    {
        perror("shmget -- sender -- create");
        exit(1);
    }

    if ((sharedMemory = (struct MEMORY *)shmat(shmid, NULL, 0)) == (struct MEMORY *)-1) /* attach pointer to shared memory */
    {
        perror("shmptr -- sender -- attach");
        exit(1);
    }

    /* Save number of columns and rows to shared memory */
    sharedMemory->numOfColumns = numOfColumns;
    sharedMemory->numOfRows = numOfRows;

    shmdt(sharedMemory); /* detach shared memory */
}

/* This function is used to fork the children and send the columns using a message queue */
void sendColumnToChildren()
{
    /* open message queue to send columns to children */
    if ((key = ftok(".", Q_SEED)) == -1)
    {
        perror("Child: key generation");
        exit(1);
    }

    if ((mid = msgget(key, 0)) == -1)
    {
        perror("Child: Queue openning");
        exit(2);
    }

    /* create children as the number of columns */
    for (int i = 0; i < numOfColumns; i++)
    {
        pid_t pid = createProcesses("./child"); /* fork child */
        if (pid == -1)
        {
            cout << "Failed\n";
            exit(1);
        }
        msg.msg_to = pid;
        string column = to_string(i + 1); // one indexed column number

        /* convert column content to a single string */
        for (unsigned j = 0; j < tokens.size(); j++)
        {
            column += " " + tokens[j][i];
        }

        /* Send column to the created child */
        memset(msg.buffer, 0x0, BUFSIZ * sizeof(char));
        strcpy(msg.buffer, column.c_str());
        msgsnd(mid, &msg, strlen(msg.buffer), 0);
    }

    /* wait for children to finish and exit if any of them fails */
    for (int i = 0; i < numOfColumns; i++)
    {
        int status;
        wait(&status);
        if (status != 0)
        {
            cout << "Child Failed\n";
            exit(1);
        }
    }
}