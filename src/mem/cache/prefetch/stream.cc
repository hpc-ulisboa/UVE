#include "debug/HWPrefetch.hh"
#include "mem/cache/prefetch/stream.hh"

StreamPrefetcher::StreamPrefetcher(const StreamPrefetcherParams *p)
: QueuedPrefetcher(p),
  tableSize(p->tableSize),
  useMasterId(p->use_master_id),
  degree(p->degree),
  distance(p->distance) {
    for(int i=0; i<MaxContexts; i++) {
        StreamTable[i] = new StreamTableEntry*[tableSize];
        for(int j=0; j<tableSize; j++) {
            StreamTable[i][j] = new StreamTableEntry[tableSize];
            StreamTable[i][j]->LRU_index = j;
            resetEntry(StreamTable[i][j]);
        }
    }
}
StreamPrefetcher::~StreamPrefetcher() {
     for (int i = 0; i < MaxContexts; i++) {
            for (int j = 0; j < tableSize; j++) {
                delete[] StreamTable[i][j];
            }
        }
};

// Training and Prefetching of streams
void
StreamPrefetcher::calculatePrefetch(const PrefetchInfo &pfi,
                                    std::vector<AddrPriority> &addresses) {
    uint32_t core_id = pfi.getMasterId() ? pfi.getMasterId() : 0;
    if (!pfi.hasPC()) {
        DPRINTF(HWPrefetch, "ignoring request with no core ID");
        return;
    }
    Addr blk_addr =
        pfi.getAddr() & ~(Addr)(blkSize - 1);  // cache block aligned address.
    assert(core_id < MaxContexts);
    StreamTableEntry** table;
    table = StreamTable[core_id];                          // Per core stream training.
    uint32_t i;
    // Check if there is a stream entry with the same address as blk_addr
    for (i = 0; i < tableSize; i++) {
        switch (table[i]->status) {
        case MONITOR:
            if(table[i]->trainedDirection == ASCENDING) {
                // Ascending order
                if((table[i]->startAddr < blk_addr ) && ( table[i]->endAddr > blk_addr)) {
                    // Hit to a stream, which is monitored. Issue prefetch requests based on the degree and the direction
                    for (uint8_t d = 1; d <= degree; d++) {
                        Addr pf_addr = table[i]->endAddr + blkSize * d;
                        addresses.push_back(AddrPriority(pf_addr,0));
                        DPRINTF(HWPrefetch, "Queuing prefetch to %#x.\n", pf_addr);
                    }
                    if((table[i]->endAddr + blkSize * degree) - table[i]->startAddr <= distance) {
                        table[i]->endAddr   = table[i]->endAddr + blkSize * degree;
                    } else {
                        table[i]->startAddr = table[i]->startAddr + blkSize * degree;
                        table[i]->endAddr   = table[i]->endAddr   + blkSize * degree;
                    }
                    break;
                }
            } else if(table[i]->trainedDirection == DESCENDING) {
                // Descending order
                if((table[i]->startAddr > blk_addr ) && (table[i]->endAddr < blk_addr)) {
                    for (uint8_t d = 1; d <= degree; d++) {
                        Addr pf_addr = table[i]->endAddr - blkSize * d;
                        addresses.push_back(AddrPriority(pf_addr,0));
                        DPRINTF(HWPrefetch, "Queuing prefetch to %#x.\n", pf_addr);
                    }
                    if(table[i]->startAddr - (table[i]->endAddr - blkSize * degree) <= distance){
                        table[i]->endAddr   = table[i]->endAddr - blkSize * degree;
                    } else {
                        table[i]->startAddr = table[i]->startAddr - blkSize * degree;
                        table[i]->endAddr   = table[i]->endAddr   - blkSize * degree;
                    }
                    break;
                }
            } else{
                assert(0);
            }
            break;
        case TRAINING:
            if ((abs(table[i]->allocAddr - blk_addr) <= (distance/2) * blkSize) ){
                // Check whether the address is in +/- of distance
                if(table[i]->trendDirection[0] == INVALID){
                    table[i]->trendDirection[0] = (blk_addr - table[i]->allocAddr > 0) ? ASCENDING : DESCENDING;
                } else {
                    assert(table[i]->trendDirection[1] == INVALID);
                    table[i]->trendDirection[1] = (blk_addr - table[i]->allocAddr > 0) ? ASCENDING : DESCENDING;
                    if(table[i]->trendDirection[0] == table[i]->trendDirection[1]) {
                        table[i]->trainedDirection = table[i]->trendDirection[0];
                        table[i]->startAddr = table[i]->allocAddr;
                        if(table[i]->trainedDirection != INVALID){
                            // Based on the trainedDirection (+1:Ascending, -1:Descending) update the end address of a stream
                            table[i]->endAddr = blk_addr + (table[i]->trainedDirection) * blkSize * degree;
                        }
                        // Entry is ready for issuing prefetch requests
                        table[i]->status = MONITOR;
                    } else {
                        resetEntry(table[i]);
                    }
                }
                break;
            }
            break;
        default:
            break;
        }  // End of Switch
    }  // End of for loop
    uint32_t HIT_index=i;
    int INVALID_index = tableSize;
    for (int i=0; i<tableSize; i++) {
        //find empty entry
        if(table[i]->status==INV) {
            INVALID_index = i;
            break;
        }
    }
    int TEMP_index = -1;
    int LRU_index = -1000000;
    for (int i=0; i<tableSize; i++) {
        //find empty entry
        if(table[i]->LRU_index > TEMP_index) {
            TEMP_index = table[i]->LRU_index;
            LRU_index  = i;
        }
    }
    assert(TEMP_index == tableSize - 1);
    int entry_id;
    if(HIT_index!=tableSize) {  //hit
        entry_id = HIT_index;
    } else if (INVALID_index!=tableSize) {
        //Existence of invalid streams
        assert(table[INVALID_index]->status == INV);
        table[INVALID_index]->status = TRAINING;
        table[INVALID_index]->allocAddr = blk_addr;
        entry_id = INVALID_index;
    } else {
        //Replace the LRU stream-entry
        assert(table[LRU_index]->status!=INV);
        resetEntry(table[LRU_index]);
        table[LRU_index]->status = TRAINING;
        table[LRU_index]->allocAddr = blk_addr;
        entry_id = LRU_index;
    }
    // Shifting the table entries after the eviction of lru-id
    for (int i=0; i<tableSize; i++) {
        if(table[i]->LRU_index < table[entry_id]->LRU_index){
            table[i]->LRU_index = table[i]->LRU_index + 1;
        }
    }
    table[entry_id]->LRU_index = 0;

}

void
StreamPrefetcher::resetEntry(StreamTableEntry *this_entry)
{

    this_entry->status                = INV;
    this_entry->trendDirection[0]     = INVALID;
    this_entry->trendDirection[1]     = INVALID;
    this_entry->allocAddr             = 0;
    this_entry->startAddr             = 0;
    this_entry->endAddr               = 0;
    this_entry->trainedDirection      = INVALID;

}

StreamPrefetcher*
StreamPrefetcherParams::create()
{
    return new StreamPrefetcher(this);
}
