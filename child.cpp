#include "local.h"

using namespace std;

pid_t cPid;
key_t key;
int mid, n;
MESSAGE msg;

int main(int argc, char *argv[])
{
    if ((key = ftok(".", SEED)) == -1)
    {
        perror("Child: key generation");
        exit(1);
    }
    // cout << "CHILD: " << key << endl;

    if ((mid = msgget(key, 0)) == -1)
    {
        perror("Child: Queue openning");
        exit(2);
    }
    cPid = getpid();

    if ((n = msgrcv(mid, &msg, BUFSIZ, cPid, 0)) == -1)
    {
        perror("Child: reading msg from queue");
        exit(3);
    }
    stringstream sColumn(msg.buffer);
    vector<string> words;
    int col;
    if (sColumn.good())
    {
        string substr;
        getline(sColumn, substr, ' ');
        col = stoi(substr);
        words.push_back(substr);
    }
    while (sColumn.good())
    {
        string substr;
        getline(sColumn, substr, ' ');

        string encodedStr = "";
        for (unsigned i = 0; i < substr.length(); i++)
        {
            char c = substr[i];
            if (isalpha(c))
            {
                if (isupper(c))
                {
                    c -= 'A';
                    c = (c + ((i + 1) << col)) % 26;
                    c += 'A';
                }
                else
                {
                    c -= 'a';
                    c = (c + ((i + 1) << col)) % 26;
                    c += 'a';
                }
                encodedStr += c;
            }
            else if (isdigit(c))
            {
                int temp = 1000000 - (c - '0');
                encodedStr += to_string(temp);
            }
            else
            {
                switch (c)
                {
                case '!':
                    encodedStr += "1";
                    break;
                case '?':
                    encodedStr += "2";
                    break;
                case ',':
                    encodedStr += "3";
                    break;
                case '*':
                    encodedStr += "4";
                    break;
                case ':':
                    encodedStr += "5";
                    break;
                case '%':
                    encodedStr += "6";
                    break;
                case '.':
                    encodedStr += "7";
                    break;
                default:
                    encodedStr += c;
                }
            }
        }

        words.push_back(encodedStr);
        cout << substr << "::" << encodedStr << " ";
    }
    cout << endl;

    return 0;
}