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

    if ((n = msgrcv(mid, &msg, BUFSIZ, cPid, 0)) != -1)
    {
        cout << msg.buffer << endl;
        cout << "ZFTT" << endl;
    }
    cout << "ZFTT" << endl;

    return 0;
}