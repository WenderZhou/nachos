#include "copyright.h"
#include "machine.h"
#include "system.h"

TLB::TLB(int tlbSize, TLBStrategyType displayStrategy)
{
    size = tlbSize;
    strategy = displayStrategy;
    printf("tlb size:%d,strategy:%s\n",size, TLBStrategyNames[strategy]);
    entry = new TranslationEntry[size];
    for (int i = 0; i < size; i++)
	    entry[i].valid = FALSE;
    FIFOtime = new int[size];
    LRUtime = new int[size];
    tlbtime = 0;
}

TLB::~TLB()
{
    delete entry;
    delete FIFOtime;
    delete LRUtime;
}

TranslationEntry*
TLB::findEnrty(int vpn)
{
    for (int i = 0; i < size; i++)
    	if (entry[i].valid && (entry[i].virtualPage == vpn))
        {
            LRUtime[i] = tlbtime++;
            return &entry[i];
        }   
    return NULL;
}

void
TLB::tlbMissHandler(int vpn)
{
    int position = -1;
    for(int i = 0; i < size; ++i){
        if(entry[i].valid == false)
        {
            position = i;
            break;
        }
    }
    if(position == -1)
    {
        switch(strategy)
        {
            case FIFO:
                position = 0;
                for(int i = 0; i < size; ++i)
                    if(FIFOtime[i] < FIFOtime[position])
                        position = i;
            break;
            case LRU:
                position = 0;
                for(int i = 0; i < size; ++i)
                    if(LRUtime[i] < LRUtime[position])
                        position = i;
            break;
        }
    }
    entry[position] = machine->pageTable[vpn];
    entry[position].valid = true;
    FIFOtime[position] = tlbtime;
    LRUtime[position] = tlbtime;
    tlbtime++;
    return;
}

void TLB::Clear()
{
    for(int i = 0; i < size; ++i)
        entry[i].valid = false;
}