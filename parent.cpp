#include "local.h"
using namespace std;

void readFile();
pid_t createProcesses(char *);
void sendColumnToChildren();

vector<vector<string>> tokens;
int maxColumns = 0;
key_t key;
int mid, n;
MESSAGE msg;

int main(int argc, char *argv[])
{
    readFile();
    sendColumnToChildren();
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
        maxColumns = max(maxColumns, (int)words.size());
        tokens.push_back(words);
    }
    for (unsigned i = 0; i < tokens.size(); i++)
    {
        while ((int)tokens[i].size() < maxColumns)
        {
            tokens[i].push_back("alright");
        }
    }

    // vector<vector<string> > columns(tokens[0].size(), vector<string>(tokens.size()));
    // for (int i = 0; i < tokens[0].size(); i++)
    //     for (int j = 0; j < data.size(); j++) {
    //         columns[i][j] = tokens[j][i];
    //     }
}

void sendColumnToChildren()
{
    if ((key = ftok(".", SEED)) == -1)
    {
        perror("Parent: key generation");
        exit(3);
    }
    // cout << "PARENT: " << key << endl;

    if ((mid = msgget(key, IPC_CREAT | 0660)) == -1) // check for IPC_CREAT flag *********
    {
        perror("Queue creation");
        exit(4);
    }

    for (int i = 0; i < maxColumns; i++)
    {
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

    // this code must be removed, and instead, shildren should inform the parent when they finish, and then the parent should remove the queue
    for (int i = 0; i < maxColumns; i++)
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
        exit(7);

    case 0:                               /* In the child */
        execlp(file, file, (char *)NULL); /* execute passed file */
        perror("exec failure ");
        exit(8);
        break;

    default:        /* In the parent */
        return pid; /* save the PID of the child */
    }
}