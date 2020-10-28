#ifndef __MEM_CACHE_PREFETCH_STREAM_HH__
#define __MEM_CACHE_PREFETCH_STREAM_HH__

#include "mem/cache/prefetch/queued.hh"
#include "params/StreamPrefetcher.hh"
// Direction of stream for each stream entry in the stream table
enum StreamDirection{
        ASCENDING = 1,                      // For example - A, A+1, A+2
        DESCENDING = -1,                    // For example - A, A-1, A-2
        INVALID = 0
};
// Status of a stream entry in the stream table.
enum StreamStatus{
            INV       = 0,
            TRAINING  = 1,                  // Stream training is not over yet. Once trained will move to MONITOR status
            MONITOR   = 2                   // Monitor and Request: Stream entry ready for issuing prefetch requests
};

class StreamPrefetcher : public QueuedPrefetcher {
  protected:
    static const uint32_t MaxContexts = 64; // Creates per-core stream tables for upto 64 processor cores
    uint32_t tableSize;                     // Number of entries in a stream table
    const bool useMasterId;                 // Use the master-id to train the streams
    uint32_t degree;                        // Determines the number of prefetch reuquests to be issued at a time
    uint32_t distance;                      // Determines the prefetch distance

   /* StreamTableEntry
     Stores the basic attributes of a stream table entry.
   */
  class StreamTableEntry {

      public:
        int  LRU_index;
        Addr allocAddr;                     // Address that initiated the stream training
        Addr startAddr;                     // First address of a stream
        Addr endAddr;                       // Last address of a stream
        StreamDirection trainedDirection;   // Direction of trained stream (Ascending or Descending)
        StreamStatus    status;             // Status of the stream entry
        StreamDirection trendDirection[2];  // Stores the last two stream directions of an entry

  };
  void resetEntry (StreamTableEntry *this_entry);

  /* Creating a StreamTable for each core with
     Tablesize as the number of stream entries
  */
  StreamTableEntry **StreamTable[MaxContexts];

  public:
  StreamPrefetcher(const StreamPrefetcherParams *p);
  ~StreamPrefetcher();
  /* Function called by cache controller to initiate
     the stream training process
  */
  void calculatePrefetch(const PrefetchInfo &pfi,
                         std::vector<AddrPriority> &addresses) override;
};

#endif // __MEM_CACHE_PREFETCH_STREAM_HH__
