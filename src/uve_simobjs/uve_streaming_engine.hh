#ifndef __UVE_STREAMING_ENGINE_HH__
#define __UVE_STREAMING_ENGINE_HH__

#include <unordered_map>

#include "arch/riscv/tlb.hh"
#include "cpu/thread_context.hh"
#include "cpu/translation.hh"
#include "debug/JMDEVEL.hh"
#include "debug/UVESE.hh"
#include "dev/io_device.hh"
#include "mem/mem_object.hh"
#include "mem/packet_access.hh"
#include "params/UVEStreamingEngine.hh"
#include "sim/process.hh"
#include "sim/system.hh"
#include "uve_simobjs/utils.hh"
#include "uve_simobjs/uve_ff_load.hh"

class SEprocessing;

class RequestExecuteEvent : public Event
{
  private:
    SEprocessing *callee;
    SERequestInfo info;
  public:
    RequestExecuteEvent(SEprocessing *callee, SERequestInfo info) :
        Event(Default_Pri, AutoDelete), callee(callee), info(info)
    { }
    void process() override;
};

// class SCftch;
// class SCmem;
class SEprocessing : SimObject
{
   private:
    class MemoryWriteHandler {
       private:
        using AddrQueue = std::list<std::pair<StreamID, SERequestInfo *>>;
        SEprocessing *owner;
        CoreContainer *data_block;
        bool has_data_block;
        uint64_t data_block_index;
        AddrQueue addr_queue;
        uint8_t *partial_data;
        bool has_partial_data;
        uint16_t partial_data_size;
        SERequestInfo *delayed_address;

       private:
        SmartReturn write_mem(SERequestInfo *info);
        SmartReturn write_mem_partial();
        SmartReturn consume_data_unit(uint64_t sz);
        SmartReturn consume_addr_unit();
        SmartReturn consume_data();

       public:
        MemoryWriteHandler()
            : owner(nullptr),
              data_block(nullptr),
              has_data_block(false),
              data_block_index(0),
              addr_queue(),
              partial_data(nullptr),
              has_partial_data(false),
              partial_data_size(0),
              delayed_address(nullptr){};
        void tick();
        void set_owner(SEprocessing *_owner) { owner = _owner; }
        void queueMemoryWrite(SERequestInfo info);
        void squash(StreamID sid);
    };

   protected:
    UVEStreamingEngine *parent;
    std::array<SEIterPtr,32> iterQueue;
    std::array<SubStreamID, 32> ssidArray;
    uint8_t load_pID, store_pID;

   public:
    void tick();

  private:
    RiscvISA::TLB *tlb;
    const Addr offsetMask;
    unsigned cacheLineSize;
    MemoryWriteHandler write_boss;

    // SCmem *memCore;
    // SCftch *fetchCore;

   public:
    // void setMemCore(SCmem * memCorePtr);

    /** constructor
     */
    SEprocessing(UVEStreamingEngineParams *params,
                 UVEStreamingEngine *_parent);

    bool setIterator(StreamID sid, SEIterPtr iterator){
      iterator->setCompareFunction(
        [=](Addr a, Addr b){return this->samePage(a,b);}
        );
      if (iterQueue[sid]->empty()) {
          delete iterQueue[sid];
          iterQueue[sid] = iterator;
          return true;
      }
      else {
        return false;
      }
    }

    bool isSinglePage(Addr addr, int size) {
        return this->samePage(addr, addr + size - 1);
    }

    void finishTranslation(WholeTranslationState *state);
    bool isSquashed() { return false; }

    void executeRequest(SERequestInfo info);
    void sendDataRequest(RequestPtr req, uint8_t *data, bool read,
                         bool split_not_last = false);
    void sendSplitDataRequest(const RequestPtr &req1, const RequestPtr &req2,
                              const RequestPtr &req, uint8_t *data, bool read);
    void accessMemory(Addr addr, int size, StreamID sid, SubStreamID ssid,
                      BaseTLB::Mode mode, uint8_t *data, ThreadContext *tc);
    void recvData(PacketPtr pkt);
    SmartReturn isCompleted(StreamID sid) { return iterQueue[sid]->ended(); }

   private:
    void emitRequest(SERequestInfo info);
    Addr pageAlign(Addr a)  { return (a & ~offsetMask); }
    Addr splitAddressOnPage(Addr a, int sz) { return pageAlign(a + sz); }
    void tick_load();
    void tick_store();

   public:
    bool samePage(Addr a, Addr b) { return (pageAlign(a) == pageAlign(b)); }
    SmartReturn clear(StreamID sid) {
        delete iterQueue[sid];
        iterQueue[sid] = new SEIter();
        this->ssidArray[sid] = -1;
        write_boss.squash(sid);
        return SmartReturn::ok();
    }
};

/*
 * Config uCore Object
 */
class SEcontroller
{
  protected:
    UVEStreamingEngine *parent;
    std::array<SEStack,32> cmdQueue;
    SEprocessing *memCore;
    // SCftch *fetchCore;

  public:
    // JMTODO: Define methods for Pio Port
   SmartReturn recvCommand(SECommand command);

   void setMemCore(SEprocessing *memCorePtr);

   /** constructor
    */
   SEcontroller(UVEStreamingEngineParams *params, UVEStreamingEngine *_parent);
};

/*
 * Main Component Object
 */

class UVEStreamingEngine : public ClockedObject
{

  /**
  * Port on the memory-side that receives responses.
  * Requests data from memory
  */
  class MemSidePort : public QueuedMasterPort
  {
    private:
      /// The object that owns this object (SCmem)
      UVEStreamingEngine *owner;

      ReqPacketQueue queue;
      SnoopRespPacketQueue snoopQueue;

      /// If we tried to send a packet and it was blocked, store it here
      // PacketPtr blockedPacket;

    public:
      /**
       * Constructor. Just calls the superclass constructor.
       */
      MemSidePort(const std::string& name, UVEStreamingEngine *owner) :
          QueuedMasterPort(name, owner, queue, snoopQueue), owner(owner),
          queue(*owner, *this), snoopQueue(*owner, *this)
      { }

      /**
       * Send a packet across this port. This is called by the owner and
       * all of the flow control is hanled in this function.
       * This is a convenience function for the SCmem to send pkts.
       *
       * @param packet to send.
       */
      // void sendPacket(PacketPtr pkt);

    protected:
      /**
       * Receive a timing response from the slave port.
       */
      bool recvTimingResp(PacketPtr pkt) override;

      /**
       * Called to receive an address range change from the peer slave
       * port. The default implementation ignores the change and does
       * nothing. Override this function in a derived class if the owner
       * needs to be aware of the address ranges, e.g. in an
       * interconnect component like a bus.
       */
      void recvRangeChange() override;
  };

  public:
    void tick();

  private:
    MemSidePort memoryPort;
    // SCftch fetchCore;
    // SCmem  memCore;
    // JMFIXME: Add port for fetch Core

  public:
    SEprocessing memCore;
    SEcontroller confCore;
    UVELoadFifo ld_fifo;
    UVEStoreFifo st_fifo;

   protected:
    Addr confAddr;
    Addr confSize;
    Tick cycler;
    std::function<void(CallbackInfo)> callback;

   public:
    /** constructor
     */
    UVEStreamingEngine(UVEStreamingEngineParams *params);

    void init();

    AddrRangeList getAddrRanges() const;
    void sendRangeChange() const;

    Port& getPort(const std::string& if_name, PortID idx) override;
    MemSidePort * getMemPort(){return &memoryPort;}

    SmartReturn recvCommand(SECommand cmd);
    Tick nextAvailableCycle(){
      if (cycler < curTick()){
        cycler = nextCycle();
        return cycler;
      }
      else{
        Tick aux = cycler;
        cycler += cyclesToTicks(Cycles(1));
        return aux;
      }
    }

    void regStats() override;
    void set_callback(void (*callback)(CallbackInfo info));
    // void send_data_to_sei(int physIdx, CoreContainer *cnt) {
    //     callback(physIdx, cnt);
    // }
    void signal_cpu(CallbackInfo info) { callback(info); }
    SmartReturn endStreamFromSquash(StreamID sid);

    SmartReturn commitLoad(StreamID sid);
    SmartReturn squashLoad(StreamID sid);
    SmartReturn shouldSquashLoad(StreamID sid);
    void synchronizeLoadLists(StreamID sid);
    SmartReturn endStreamLoad(StreamID sid);
    SmartReturn getDataLoad(StreamID sid);

    SmartReturn commitStore(StreamID sid, SubStreamID ssid);
    SmartReturn squashStore(StreamID sid, SubStreamID ssid);
    SmartReturn shouldSquashStore(StreamID sid);
    void synchronizeStoreLists(StreamID sid);
    SmartReturn endStreamStore(StreamID sid);
    void setDataStore(StreamID sid, SubStreamID ssid, CoreContainer val);
    SubStreamID reserveStoreEntry(StreamID sid) {
        SubStreamID ssid_static,
            *ssid = (SubStreamID *)malloc(sizeof(SubStreamID));
        auto result = st_fifo.reserve(sid, ssid);
        ssid_static = *ssid;
        free(ssid);
        return ssid_static;
    }

    SmartReturn isFinished(StreamID sid) {
        return ld_fifo.isFinished(sid).AND(memCore.isCompleted(sid));
    }
};

#endif // __UVE_STREAMING_ENGINE_HH__
