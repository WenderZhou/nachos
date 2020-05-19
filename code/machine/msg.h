#ifndef MSG_H
#define MSG_H

#include "queue"

#define MSG_TYPE_NUM 5
#define MSG_QUEUE_NUM 10

struct Msg
{
    int type;
    int size;
    char* buffer;
};

class MsgQueue
{
public:
    MsgQueue(int _key);
    int Add(char* msgp, int msgsz, int msgtyp);
    int Remove(char* msgp, int msgsz, int msgtyp);
    int key;
private:
    std::queue<Msg*> queue[MSG_TYPE_NUM];
};

class MsgQueueManager
{
public:
    MsgQueueManager();
    int CreateQueue(int key);
    int Snd(int msgid, char* msgp, int msgsz, int msgtyp);
    int Rcv(int msgid, char* msgp, int msgsz, int msgtyp);
private:
    MsgQueue* msgQueue[MSG_QUEUE_NUM];
};

#endif