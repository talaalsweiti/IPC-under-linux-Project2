#include "local.h"

using namespace std;

pid_t cPid;
key_t key;
int mid, n;
MESSAGE msg;

void decode(string);

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
                    c = (c + ((i + 1) * col)) % 26;
                    c += 'A';
                }
                else
                {
                    c -= 'a';
                    c = (c + ((i + 1) * col)) % 26;
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
        // cout << substr << "::" << encodedStr << " ";
    }
    // cout << endl;

    string encodedColumn = "";
    for (unsigned k = 0; k < words.size(); k++)
    {
        encodedColumn += words[k] + " ";
    }
    decode(encodedColumn);
    return 0;
}

void decode(string encodedColumn)
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
        }
        words.push_back(decodedStr);
        cout << substr << "::" << decodedStr << " ";
    }
    cout << endl;
}