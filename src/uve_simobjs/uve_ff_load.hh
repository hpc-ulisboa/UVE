#ifndef __UVE_SIMOBJS_FIFO_LOAD_HH__
#define __UVE_SIMOBJS_FIFO_LOAD_HH__

#include "arch/riscv/registers.hh"
#include "base/circlebuf.hh"
#include "debug/JMDEVEL.hh"
#include "params/UVEStreamingEngine.hh"
#include "sim/clocked_object.hh"
#include "sim/system.hh"
#include "uve_simobjs/fifo_utils.hh"
#include "uve_simobjs/utils.hh"

using CoreContainer = RiscvISA::VecRegContainer;
using ViewContainer = RiscvISA::VecReg;

class UVELoadFifo;

// Each fifo entry is a VecRegContainer extended with valid bits and
// a size counter.. this is to enable the ordered fill of each container
class FifoEntry : public CoreContainer {
   private:
    typedef enum { Complete, NotComplete, Clean } States;
    States rstate, cstate;
    uint16_t size, csize;
    uint16_t config_size;

   public:
    FifoEntry(uint8_t width, uint16_t _cfg_sz)
        : CoreContainer(),
          size(0),
          csize(0),
          config_size(_cfg_sz / 8) {  // Config size to be used in bytes
        this->zero();
        rstate = States::NotComplete;
        cstate = States::Clean;
        set_width(width);
    }
    bool complete() { return rstate == States::Complete; }
    bool ready() { return cstate == States::Complete; }

    void merge_data(uint8_t *data, uint16_t offset, uint16_t size);
    uint16_t getSize() { return size; }
    bool reserve(uint16_t _size, bool last);
};

// Each fifo is composed of FifoEntry objects, which themselfs insert the data
// and verify themselfs as destination.
// If the data is not for the current entries, create another entry.
// StreamFifo only redirects the data, and gives statistics on how full it is
class StreamFifo : protected std::vector<FifoEntry> {
   private:
    typedef struct mapEntry {
        uint16_t id;
        uint16_t size;
        uint16_t offset;
        bool used;
    } MapStruct;

    MapStruct create_MS(uint16_t id, uint16_t size, uint16_t offset) {
        MapStruct new_ms;
        new_ms.id = id;
        new_ms.size = size;
        new_ms.offset = offset;
        new_ms.used = false;
        return new_ms;
    }

    uint8_t max_size;
    UVELoadFifo *owner;
    std::vector<MapStruct> map;
    std::list<int> physRegStack;
    uint16_t config_size;
    SEIterationStatus status;

   public:
    StreamFifo(uint16_t _cfg_sz)
        : max_size(FIFO_DEPTH),
          owner(nullptr),
          physRegStack(),
          config_size(_cfg_sz),
          status(SEIterationStatus::Clean) {
        reserve(max_size);
    }

    void set_owner(UVELoadFifo *own) { owner = own; }
    void insert(uint16_t size, uint16_t ssid, uint8_t width, bool last);
    bool merge_data(uint16_t ssid, uint8_t *data);
    FifoEntry get();
    FifoEntry get(int *physidx);
    bool full();
    bool ready();
    bool ready(int physidx);
    bool squash(int physidx);
    bool empty();
    bool complete() { return status == SEIterationStatus::Ended; }

    void insert(int physIdx);

   private:
    uint16_t real_size();
};

// This is the load fifo object that contains one fifo per stream
class UVELoadFifo : public SimObject {
   private:
    std::vector<StreamFifo> fifos;

   public:
    uint16_t cacheLineSize;
    UVEStreamingEngine *engine;

   public:
    UVELoadFifo(UVEStreamingEngineParams *params);

    bool tick();
    std::vector<std::pair<int, CoreContainer *>> get_data();
    void init();
    bool insert(StreamID sid, uint32_t ssid, CoreContainer data);
    void reserve(StreamID sid, uint32_t ssid, uint8_t size, uint8_t width,
                 bool last);
    bool fetch(StreamID sid, CoreContainer **cnt);
    bool full(StreamID sid);

    bool reserve(StreamID sid, int physIdx);
    bool ready(StreamID sid, int physIdx);
    bool squash(StreamID sid, int physIdx);
    bool isFinished(StreamID sid);
};

#endif  //__UVE_SIMOBJS_FIFO_LOAD_HH__