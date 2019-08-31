#ifndef __UVE_STREAMING_ENGINE_HH__
#define __UVE_STREAMING_ENGINE_HH__

#include <unordered_map>

#include "debug/JMDEVEL.hh"
#include "debug/UVESE.hh"
#include "dev/io_device.hh"
#include "mem/mem_object.hh"
#include "params/UVEStreamingEngine.hh"
#include "sim/system.hh"
#include "uve_simobjs/utils.hh"

// class SCftch;
// class SCmem;
class SEprocessing
{
  protected:
    UVEStreamingEngine *parent;
    std::array<SEIterPtr,32> iterQueue;
    uint8_t pID;
    // SCmem *memCore;
    // SCftch *fetchCore;

  public:

    // void setMemCore(SCmem * memCorePtr);

    /** constructor
     */
    SEprocessing(UVEStreamingEngineParams *params,
                UVEStreamingEngine *_parent){
                  parent = _parent;
                  iterQueue.fill(new SEIter());
                };

    bool setIterator(StreamID sid, SEIterPtr iterator){
      //JMFIXME: Clean memory on past iterators;
      if(iterQueue[sid]->empty()){
        iterQueue[sid] = iterator;
        return true;
      }
      else {
        iterQueue[sid] = iterator;
        return false;
      }
    }

    void Tick();

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
  class MemSidePort : public MasterPort
  {
    private:
      /// The object that owns this object (SCmem)
      UVEStreamingEngine *owner;

      /// If we tried to send a packet and it was blocked, store it here
      // PacketPtr blockedPacket;

    public:
      /**
       * Constructor. Just calls the superclass constructor.
       */
      MemSidePort(const std::string& name, UVEStreamingEngine *owner) :
          MasterPort(name, owner), owner(owner)
      { }

      /**
       * Send a packet across this port. This is called by the owner and
       * all of the flow control is hanled in this function.
       * This is a convenience function for the SCmem to send pkts.
       *
       * @param packet to send.
       */
      void sendPacket(PacketPtr pkt);

    protected:
      /**
       * Receive a timing response from the slave port.
       */
      bool recvTimingResp(PacketPtr pkt) override;

      /**
       * Called by the slave port if sendTimingReq was called on this
       * master port (causing recvTimingReq to be called on the slave
       * port) and was unsuccesful.
       */
      void recvReqRetry() override;

      /**
       * Called to receive an address range change from the peer slave
       * port. The default implementation ignores the change and does
       * nothing. Override this function in a derived class if the owner
       * needs to be aware of the address ranges, e.g. in an
       * interconnect component like a bus.
       */
      void recvRangeChange() override;
  };

  class CpuSidePort : public SimpleTimingPort
  {
    protected:
    /** The device that this port serves. */
    UVEStreamingEngine *device;

    virtual Tick recvAtomic(PacketPtr pkt);

    virtual AddrRangeList getAddrRanges() const;

    public:
      CpuSidePort(const std::string& name, UVEStreamingEngine *dev) :
          SimpleTimingPort(name, dev), device(dev)
      {}
  };
  public:
    void tick();

  private:
    MemSidePort memoryPort;
    CpuSidePort confPort;
    // SCftch fetchCore;
    // SCmem  memCore;
    // JMFIXME: Add port for fetch Core

  public:
    SEprocessing memCore;
    SEcontroller confCore;

  protected:
    Addr confAddr;
    Addr confSize;

  public:
  /** constructor
   */
    UVEStreamingEngine(UVEStreamingEngineParams *params);

    void init() override;

    //PioDevice Methods
    Tick read(PacketPtr pkt);
    Tick write(PacketPtr pkt);
    AddrRangeList getAddrRanges() const;
    void sendRangeChange() const;

    Port& getPort(const std::string& if_name, PortID idx) override;

    bool recvCommand(SECommand cmd);

};


#endif // __UVE_STREAMING_ENGINE_HH__
