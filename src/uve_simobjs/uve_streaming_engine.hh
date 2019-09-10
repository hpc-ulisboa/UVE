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
  protected:
    UVEStreamingEngine *parent;
    std::array<SEIterPtr,32> iterQueue;
    uint8_t pID;

  public:
    void tick();

  private:
    RiscvISA::TLB *tlb;
    const Addr offsetMask;
    unsigned cacheLineSize;

    // SCmem *memCore;
    // SCftch *fetchCore;

  public:

    // void setMemCore(SCmem * memCorePtr);

    /** constructor
     */
    SEprocessing(UVEStreamingEngineParams *params,
                UVEStreamingEngine *_parent);

    bool setIterator(StreamID sid, SEIterPtr iterator){
      //JMFIXME: Clean memory on past iterators;
      iterator->setCompareFunction(
        [=](Addr a, Addr b){return this->samePage(a,b);}
        );
      if (iterQueue[sid]->empty()){
        iterQueue[sid] = iterator;
        return true;
      }
      else {
        iterQueue[sid] = iterator;
        return false;
      }
    }
    void finishTranslation(WholeTranslationState *state);
    bool isSquashed() { return false; }

    void executeRequest(SERequestInfo info);
    void sendData(RequestPtr req, uint8_t *data, bool read);
    void accessMemory(Addr addr, int size, int sid, int ssid,
                      BaseTLB::Mode mode, uint8_t *data,
                      ThreadContext* tc);
    void recvData(PacketPtr pkt);

  private:
    void emitRequest(SERequestInfo info);
    Addr pageAlign(Addr a)  { return (a & ~offsetMask); }
  public:
    bool samePage(Addr a, Addr b) { return (pageAlign(a)==pageAlign(b));}
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
    Tick recvCommand(SECommand command);

    void setMemCore(SEprocessing * memCorePtr);

    /** constructor
     */
    SEcontroller(UVEStreamingEngineParams *params,
                UVEStreamingEngine *_parent);
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
    ThreadContext * context;
    UVELoadFifo ld_fifo;

  protected:
    Addr confAddr;
    Addr confSize;
    Tick cycler = 0;

  public:
  /** constructor
   */
    UVEStreamingEngine(UVEStreamingEngineParams *params);

    void init();

    //PioDevice Methods
    Tick read(PacketPtr pkt);
    Tick write(PacketPtr pkt);
    AddrRangeList getAddrRanges() const;
    void sendRangeChange() const;

    Port& getPort(const std::string& if_name, PortID idx) override;
    MemSidePort * getMemPort(){return &memoryPort;}

    bool recvCommand(SECommand cmd);
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


};


#endif // __UVE_STREAMING_ENGINE_HH__
