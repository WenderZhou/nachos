#include "copyright.h"
#include "utility.h"
#include "translate.h"
#include "bitmap.h"
#include "openfile.h"

#define SWAP_SPACE_NAME "SWAPSPACE"
#define SWAP_SPACE_SIZE 100

class SwapSpace{
public:
    SwapSpace();
    ~SwapSpace();
    int SwapIn(int physicalPage);
    void SwapOut(int physicalPage, int swapPage);
    void Clear(int swapPage){ bitMap->Clear(swapPage); }
private:
    OpenFile *file; // swap space file on disk
    BitMap *bitMap; // recode the usage of swap space
};