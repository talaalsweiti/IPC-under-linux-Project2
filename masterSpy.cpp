#include "local.h"

using namespace std;
int numOfColumns, numOfRows;
int mid;
int NUM_OF_SPIES;

void decode(string, string[]);
void writeToFile(char columns[][MAX_STRING_LENGTH]);
void openMessageQueue();
void getDimensions();

int main()
{
    char cols[numOfColumns][MAX_STRING_LENGTH];
    bool isExist[numOfColumns] = {false};

    int cntCols = 0;
    // Access and modify the shared matrix, to get all columns
    while (cntCols < numOfColumns)
    {
    }

    shmdt(sharedMemory);

    writeToFile(cols);

    // shmctl(shmid, IPC_RMID, (struct shmid_ds *)0);
    // Other processes can now attach to the same shared memory segment using the same key
    return 0;
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