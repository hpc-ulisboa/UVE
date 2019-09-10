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

using SmartContainer = std::pair<
                        CoreContainer,
                        EnableContainer
                        >;

class UVELoadFifo;

//Each fifo entry is a VecRegContainer extended with valid bits and
// a size counter.. this is to enable the ordered fill of each container
class FifoEntry : public CoreContainer {
    private:
        EnableContainer enable_bits;
        uint16_t size;
        uint16_t ssid;
        uint8_t width;

    public:
        FifoEntry() : CoreContainer(), size(0), ssid(0), width(0)
        {
            enable_bits.fill(false);
        }
        FifoEntry(CoreContainer cnt, uint8_t _size, uint16_t _ssid,
                    uint8_t _width) :
                CoreContainer(), size(_size), ssid(_ssid), width(_width)
        {
            enable_bits.fill(false);
            std::fill_n(enable_bits.begin(),_size, true);

            auto my_view = this->as<uint8_t>();
            auto else_view = cnt.as<uint8_t>();
            for (int i=0; i<_size; i++)  my_view[i] = else_view[i];
        }
        bool complete(){ return size == SIZE; }

        bool enabled(int i){ return enable_bits[i]; }
        EnableContainer getEnable(){return enable_bits;};
        uint16_t getSsid() {return ssid;}
        void merge_entry(FifoEntry new_entry, uint16_t offset);
};

//Each fifo is composed of FifoEntry objects, which themselfs insert the data
// and verify themselfs as destination.
//If the data is not for the current entries, create another entry.
//StreamFifo only redirects the data, and gives statistics on how full it is
class StreamFifo : protected std::vector<FifoEntry> {
    private:
        uint8_t max_size;
        UVELoadFifo * owner;

    public:
        StreamFifo() : max_size(FIFO_DEPTH), owner(nullptr)
        { reserve(max_size); }

        void set_owner(UVELoadFifo * own){owner = own;}
        void insert(FifoEntry entry);
        bool merge(FifoEntry entry);
        FifoEntry get();
        bool full();
        bool ready();
        bool empty();
};

//This is the load fifo object that contains one fifo per stream
class UVELoadFifo : public SimObject {
    private:
        std::array<StreamFifo, 32> fifos;
    public:
        uint16_t cacheLineSize;
    public:
        UVELoadFifo(UVEStreamingEngineParams *params);

        void tick();
        void init();
        bool insert(StreamID sid, uint32_t ssid, CoreContainer data,
                    uint8_t size, uint8_t width);
        bool fetch(StreamID sid, SmartContainer * cnt);
    private:
        SmartContainer to_smart(FifoEntry entry);
};

#endif //__UVE_SIMOBJS_FIFO_LOAD_HH__