#include "copyright.h"
#include "utility.h"
#include "translate.h"

enum TLBStrategyType { FIFO, LRU };
static char* TLBStrategyNames[] = { "FIFO", "LRU" };
#define TLBStrategy	LRU

class TLB{
  public:
    TLB(int tlbSize, TLBStrategyType displayStrategy);
    ~TLB();
    void tlbMissHandler(int vpn);
    TranslationEntry* findEnrty(int vpn);
    void Clear();
  private:
    TranslationEntry *entry;
    TLBStrategyType strategy;
    int *FIFOtime;
    int *LRUtime;
    int tlbtime;
    int size;
};