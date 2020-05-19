#include "msg.h"
#include "string.h"

MsgQueue::MsgQueue(int _key)
{
    key = _key;
}

int MsgQueue::Add(char* msgp, int msgsz, int msgtyp)
{
    Msg* msg = new Msg();
    msg->type = msgtyp;
    msg->size = msgsz;
    msg->buffer = new char[msgsz];
    memcpy(msg->buffer, msgp, msgsz);
    queue[msgtyp].push(msg);
}

int MsgQueue::Remove(char* msgp, int msgsz, int msgtyp)
{
    Msg* msg = queue[msgtyp].front();
    queue[msgtyp].pop();
    if(msgsz > msg->size)
        msgsz = msg->size;
    memcpy(msgp,msg->buffer,msgsz);
    delete msg;
}

MsgQueueManager::MsgQueueManager()
{
    for(int i = 0; i < MSG_QUEUE_NUM; i++)
        msgQueue[i] = NULL;
}

int MsgQueueManager::CreateQueue(int key)
{
    for(int i = 0; i < MSG_QUEUE_NUM; i++)
        if(msgQueue[i] != NULL)
            if(msgQueue[i]->key == key)
                return i;
    for(int i = 0; i < MSG_QUEUE_NUM; i++)
        if(msgQueue[i] == NULL)
        {
            msgQueue[i] = new MsgQueue(key);
            return i;
        }
    return -1;
}

int MsgQueueManager::Snd(int msgid, char* msgp, int msgsz, int msgtyp)
{
    if(msgQueue[msgid] == NULL)
        return -1;
    msgQueue[msgid]->Add(msgp,msgsz,msgtyp);
}

int MsgQueueManager::Rcv(int msgid, char* msgp, int msgsz, int msgtyp)
{
    if(msgQueue[msgid] == NULL)
        return -1;
    msgQueue[msgid]->Remove(msgp,msgsz,msgtyp);
}