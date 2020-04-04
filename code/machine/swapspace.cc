#include "copyright.h"
#include "system.h"

//----------------------------------------------------------------------
//  SwapSpace::SwapSpace()
// 	Initialize a swap space, to store those displayed from memory
//----------------------------------------------------------------------
SwapSpace::SwapSpace()
{
    fileSystem->Create(SWAP_SPACE_NAME, SWAP_SPACE_SIZE * PageSize);
    file = fileSystem->Open(SWAP_SPACE_NAME);
    bitMap = new BitMap(SWAP_SPACE_SIZE);
}

//----------------------------------------------------------------------
//  SwapSpace::~SwapSpace()
// 	De-allocate the swap space
//----------------------------------------------------------------------
SwapSpace::~SwapSpace()
{
    delete file;
    fileSystem->Remove(SWAP_SPACE_NAME);
}

//----------------------------------------------------------------------
//  SwapSpace::SwapIn
// 	swap a page in memory to swap space on disk, return the swap space
//  page number if the swap succeed, otherwise return -1
//	
//	"physicalPage" is the physical page frame of the swapped page
//----------------------------------------------------------------------
int SwapSpace::SwapIn(int physicalPage)
{
    int swapPage = bitMap->Find();
    if(swapPage != -1)
    {
        int physAddr = physicalPage * PageSize;
        int swapAddr = swapPage * PageSize;
        file->WriteAt(&machine->mainMemory[physAddr], PageSize, swapAddr);
    }
    return swapPage;
}

//----------------------------------------------------------------------
//  SwapSpace::SwapOut
// 	swap a page in swap space on disk to main memory
//	
//	"physicalPage" is the target physical page frame
//  "swapPage" is the page number of the page in swap space
//----------------------------------------------------------------------
void SwapSpace::SwapOut(int physicalPage, int swapPage)
{
    ASSERT(bitMap->Test(swapPage));
    int physAddr = physicalPage * PageSize;
    int swapAddr = swapPage * PageSize;
    file->ReadAt(&machine->mainMemory[physAddr], PageSize, swapAddr);
    bitMap->Clear(swapPage);
}