#include "local.h"

using namespace std;
int numOfColumns, numOfRows;
int mid, shmid, r_shmid, mut_semid, r_semid, w_semid;
int NUM_OF_SPIES = 3;

static struct sembuf acquire = {0, -1, SEM_UNDO},
                     release = {0, 1, SEM_UNDO};

void decode(string, string[]);
void writeToFile(char columns[][MAX_STRING_LENGTH]);
void getDimensions();
void openMessageQueue();

int main()
{
    openMessageQueue();

    char cols[numOfColumns][MAX_STRING_LENGTH];
    bool isExist[numOfColumns] = {false};

    unsigned col;
    int cntCols = 0;
    // Access and modify the shared matrix, to get all columns
    while (cntCols < numOfColumns)
    {

        col = rand() % numOfColumns;
        cout << "IM RANDOM COL: " << col << endl;

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
        if (sline.good())
        {
            string colNumStr;
            getline(sline, colNumStr, ' ');
            int colNum = stoi(colNumStr);
            if (!isExist[colNum - 1])
            {
                cntCols++;
                isExist[colNum - 1] = true;

                strncpy(cols[colNum - 1], sharedMemory->data[col], MAX_STRING_LENGTH - 1);
                cols[colNum - 1][MAX_STRING_LENGTH - 1] = '\0';
                cout << colNum << " -- " << sharedMemory->data[col] << endl;
                cout << "    READING COL DONE " << endl;
            }
        }

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

    writeToFile(cols);

    // shmctl(shmid, IPC_RMID, (struct shmid_ds *)0);
    // Other processes can now attach to the same shared memory segment using the same key
    return 0;
}

void getDimensions()
{
}

void openMessageQueue()
{
    key_t key;
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
}

void decode(string encodedColumn, string decodedRows[])
{
    stringstream sColumn(encodedColumn);
    vector<string> words;
    int col;
    if (sColumn.good())
    {
        string substr;
        getline(sColumn, substr, ' ');
        col = stoi(substr);
        words.push_back(substr);
    }

    int row = 0;
    while (sColumn.good())
    {
        string substr;
        getline(sColumn, substr, ' ');

        string decodedStr = "";
        for (unsigned i = 0; i < substr.length(); i++)
        {
            char c = substr[i];
            if (isalpha(c))
            {
                if (isupper(c))
                {
                    c -= 'A';
                    c = (c - ((i + 1) * col));
                    if (c < 0)
                    {
                        int temp = ((-1 * c / 26) + 1);
                        c = temp * 26 + c;
                    }
                    c += 'A';
                }
                else
                {
                    c -= 'a';
                    c = (c - ((i + 1) * col));
                    if (c < 0)
                    {
                        int temp = ((-1 * c / 26) + 1);
                        c = temp * 26 + c;
                    }
                    c += 'a';
                }
                decodedStr += c;
            }
            else if (isdigit(c))
            {
                if (c == '1' && i + 1 < substr.length() && substr[i + 1] == '0')
                {
                    decodedStr += "0";
                    i += 6;
                    continue;
                }
                if (c == '9' && i + 5 < substr.length())
                {

                    int temp = substr[i + 5] - '0';
                    temp = 10 - temp;
                    decodedStr += to_string(temp);
                    i += 5;
                    continue;
                }

                switch (c)
                {
                case '1':
                    decodedStr += "!";
                    break;
                case '2':
                    decodedStr += "?";
                    break;
                case '3':
                    decodedStr += ",";
                    break;
                case '4':
                    decodedStr += "*";
                    break;
                case '5':
                    decodedStr += ":";
                    break;
                case '6':
                    decodedStr += "%";
                    break;
                case '7':
                    decodedStr += ".";
                    break;
                default:
                    decodedStr += c;
                }
            }
            else
            {
                decodedStr += c;
            }
        }
        if (decodedStr == "alright")
        {
            row++;
            continue;
        }
        if (col != 1)
        {
            decodedRows[row] += " ";
        }
        decodedRows[row] += decodedStr;
        row++;
        words.push_back(decodedStr);
    }
}

void writeToFile(char columns[][MAX_STRING_LENGTH])
{
    string decodedRows[numOfRows];
    for (int i = 0; i < numOfColumns; i++)
    {
        decode(columns[i], decodedRows);
    }

    ofstream receiverFile;
    receiverFile.open("receiver.txt");

    for (int i = 0; i < numOfRows; i++)
    {
        receiverFile << decodedRows[i] << "\n";
    }

    receiverFile.close();
}