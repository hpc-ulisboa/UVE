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
using EnableContainer = std::array<bool, CoreContainer::SIZE>;

using SmartContainer = std::pair<CoreContainer, EnableContainer>;

class UVELoadFifo;

// Each fifo entry is a VecRegContainer extended with valid bits and
// a size counter.. this is to enable the ordered fill of each container
class FifoEntry : public CoreContainer {
   private:
    typedef enum { Complete, NotComplete, Clean } States;
    States rstate, cstate;
    EnableContainer enable_bits;
    uint16_t size, csize;
    uint8_t width;

   public:
    FifoEntry(uint8_t _width)
        : CoreContainer(), size(0), csize(0), width(_width) {
        this->zero();
        rstate = States::NotComplete;
        cstate = States::Clean;
        enable_bits.fill(false);
    }
    bool complete() { return rstate == States::Complete; }
    bool ready() { return cstate == States::Complete; }

    bool enabled(int i) { return enable_bits[i]; }
    EnableContainer getEnable() { return enable_bits; };
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

   public:
    StreamFifo() : max_size(FIFO_DEPTH), owner(nullptr) { reserve(max_size); }

    void set_owner(UVELoadFifo *own) { owner = own; }
    void insert(uint16_t size, uint16_t ssid, uint8_t width, bool last);
    bool merge_data(uint16_t ssid, uint8_t *data);
    FifoEntry get();
    bool full();
    bool ready();
    bool ready(int physidx);
    bool empty();

    void insert(int physIdx);

   private:
    uint16_t real_size();
};

// This is the load fifo object that contains one fifo per stream
class UVELoadFifo : public SimObject {
   private:
    std::array<StreamFifo, 32> fifos;

   public:
    uint16_t cacheLineSize;

   public:
    UVELoadFifo(UVEStreamingEngineParams *params);

    void tick();
    void init();
    bool insert(StreamID sid, uint32_t ssid, CoreContainer data);
    void reserve(StreamID sid, uint32_t ssid, uint8_t size, uint8_t width,
                 bool last);
    bool fetch(StreamID sid, SmartContainer *cnt);
    bool full(StreamID sid);

    bool reserve(StreamID sid, int physIdx);
    bool ready(StreamID sid, int physIdx);

   private:
    SmartContainer to_smart(FifoEntry entry);
};

#endif  //__UVE_SIMOBJS_FIFO_LOAD_HH__