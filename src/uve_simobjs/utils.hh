#ifndef __UVE_SE_UTILS_HH__
#define __UVE_SE_UTILS_HH__

#include <queue>
#include <string>

#include <boost/serialization/strong_typedef.hpp>

#include "base/cprintf.hh"
#include "cpu/inst_seq.hh"
#include "cpu/thread_context.hh"
#include "debug/JMDEVEL.hh"
#include "sim/sim_object.hh"
#include "smart_return.hh"
#include "tree_utils.hh"

#define MaximumStreams 32
#define STOP_SIZE 512

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define PR_ANN(x) "\033[32m" x "\033[0m"
#define PR_INFO(x) "\033[36m" x "\033[0m"
#define PR_WARN(x) "\033[33m" x "\033[0m"
#define PR_ERR(x) "\033[31m" x "\033[0m"
#define PR_DBG(x) "\033[35m" x "\033[0m"


template <typename T>
T mult_all(std::vector<T> * vec){
    uint64_t result = 1;
    for (auto&& x: *vec){
        result *= x;
    }
    return (T)result;
}

typedef uint64_t DimensionSize;
typedef uint64_t DimensionOffset;
typedef uint64_t DimensionStride;
typedef uint64_t DimensionVariation;
typedef enum  {
    incr = 'i',
    decr = 'd',
    add  = 'a',
    sub  = 's',
    set  = 't'
}DimensionBehaviour;
typedef enum  {
    size = 's',
    stride = 't',
    offset = 'o'
}DimensionTarget;

typedef enum  {
    dimension = 'd',
    modifier = 'm',
    indirection = 'i'
}StreamSetting;

class SEDimension
{
    enum specification {
        complete,
        init
    };
    private:
        DimensionSize   size;
        DimensionOffset offset;
        DimensionStride stride;
        DimensionVariation variation;
        DimensionBehaviour behaviour;
        DimensionTarget target;
        StreamSetting setting;
        specification type;
    public:
        SEDimension(DimensionSize size, DimensionOffset offset,
            DimensionStride stride):
                size(size), offset(offset), stride(stride)
            {
                setting = StreamSetting::dimension;
                type = specification::complete;
            }
        SEDimension(DimensionOffset offset):
                offset(offset)
            {
                setting = StreamSetting::dimension;
                type = specification::init;
            }
        SEDimension(DimensionSize size, DimensionBehaviour behav,
                    DimensionTarget target, DimensionVariation variation):
                size(size), offset(0), stride(0), variation(variation),
                behaviour(behav), target(target)
            {
                setting = StreamSetting::modifier;
                type = specification::complete;
            }
        ~SEDimension() {}

        std::string
        to_string(){
            switch (setting)
            {
            case StreamSetting::modifier:
                return csprintf("S(%d):V(%d):B(%c):T(%c)", size, variation,
                                (char)behaviour, (char)target);
            case StreamSetting::dimension:
                return csprintf("S(%d):O(%d):St(%d)", size, offset, stride);
            default:
                return csprintf("--==Dimension not implemented==--");
            }
        }

        DimensionSize get_size() { return size; }
        DimensionStride get_stride() { return stride; }
        DimensionOffset get_offset() { return offset; }
        DimensionVariation get_variation() { return variation; }
        StreamSetting get_setting() { return setting; }
        DimensionBehaviour get_behaviour() { return behaviour; }
        DimensionTarget get_target() { return target; }

};

typedef enum  {
    load = 'L',
    store = 'S'
}StreamMode;
typedef enum {
    byte = 'B',
    half = 'H',
    word = 'W',
    dword = 'D',
}StreamWidth;
typedef enum  {
    append = 'a',
    start = 's',
    end = 'e',
    simple = 'i'
}StreamType;
typedef int8_t StreamID;
typedef uint32_t SubStreamID;
BOOST_STRONG_TYPEDEF(StreamID, ArchStreamID)
BOOST_STRONG_TYPEDEF(StreamID, PhysStreamID)

typedef struct {
    StreamID psids[32];
    int8_t psids_size;
    bool is_clear;
    StreamID clear_sid;
} CallbackInfo;

class SEStream
{
    private:
        StreamID id;
        StreamWidth width;
        StreamMode mode;
        StreamType type;
        StreamSetting setting;

    public:
        SEStream(StreamID stream, StreamWidth width, StreamMode mode,
            StreamType type):
             width(width), mode(mode), type(type)
        {
            assert(stream < MaximumStreams);
            id = stream;
            setting = StreamSetting::dimension;
        }
        //Append and End dont know width or mode
        SEStream(StreamID stream, StreamType type):
             type(type)
        {
            assert(type == StreamType::append || type == StreamType::end);
            assert(stream < MaximumStreams);
            id = stream;
            setting = StreamSetting::dimension;
        }
        //Append and End for modifiers
        SEStream(StreamID stream, StreamType type, StreamSetting _setting):
             type(type)
        {
            assert(type == StreamType::append || type == StreamType::end);
            assert(stream < MaximumStreams);
            id = stream;
            setting = _setting;
        }
        ~SEStream(){}

        std::string
        to_string(){
            switch (setting)
            {
            case StreamSetting::dimension:
                if (type == StreamType::start || type == StreamType::simple)
                return csprintf("ID(%d):T(%c):W(%c):::M(%c)::S(%c)",id,
                    (char)type, (char)width, (char)mode, (char)setting);
                else
                return csprintf("ID(%d):T(%c)::S(%c)",id, (char)type,
                                (char)setting);
            case StreamSetting::modifier:
                return csprintf("ID(%d):T(%c)::S(%c)",id, (char)type,
                                (char)setting);
            case StreamSetting::indirection:
            default:
                return csprintf("Stream not implemented for this setting");
            }
        }

        StreamID getID(){
            return id;
        }
        uint8_t getWidth(){
            uint8_t _width = 0;
            switch (width)
            {
            case StreamWidth::byte :
                _width = 1;
                break;
            case StreamWidth::half :
                _width = 2;
                break;
            case StreamWidth::word :
                _width = 4;
                break;
            case StreamWidth::dword :
                _width = 8;
                break;
            }
            return _width;
        }
        uint8_t getCompressedWidth() {
            uint8_t _width = 0;
            switch (width) {
                case StreamWidth::byte:
                    _width = 0;
                    break;
                case StreamWidth::half:
                    _width = 1;
                    break;
                case StreamWidth::word:
                    _width = 2;
                    break;
                case StreamWidth::dword:
                    _width = 3;
                    break;
            }
            return _width;
        }
        StreamMode getMode() { return mode; }
        StreamType getType(){
            return type;
        }
        StreamSetting getSetting(){
            return setting;
        }
        void setType(StreamType _type){
            if (type==start)
                type = StreamType::simple;
            else
                type = StreamType::end;
        }

};


class SECommand {
    private:
     SEStream* stream;
     SEDimension* dimension;
     ThreadContext* tc;

    public:
     SECommand() : stream(nullptr), dimension(nullptr), tc(nullptr) {}
     // Create command from stream and dimension objects
     SECommand(ThreadContext* _tc, SEStream* _stream,
               SEDimension* _dimension) {
         stream = _stream;
         dimension = _dimension;
         tc = _tc;
     }
     // Create command for simple stream
     SECommand(ThreadContext* _tc, StreamID _s, StreamWidth _w, StreamMode _m,
               DimensionOffset _o, DimensionSize _sz, DimensionStride _st,
               StreamType type = StreamType::simple) {
         stream = new SEStream(_s, _w, _m, type);
         dimension = new SEDimension(_sz, _o, _st);
         tc = _tc;
     }
     // Create command for init
     SECommand(ThreadContext* _tc, StreamID _s, StreamWidth _w, StreamMode _m,
               DimensionOffset _o) {
         stream = new SEStream(_s, _w, _m, StreamType::start);
         dimension = new SEDimension(_o);
         tc = _tc;
     }
     // Create command for simple append and end
     SECommand(ThreadContext* _tc, StreamID _s, DimensionOffset _o,
               DimensionSize _sz, DimensionStride _st,
               StreamType type = StreamType::append) {
         stream = new SEStream(_s, type);
         dimension = new SEDimension(_sz, _o, _st);
         tc = _tc;
     }
     // Create command for modified append and end
     SECommand(ThreadContext* _tc, StreamID _s, DimensionSize _sz,
               DimensionTarget _tg, DimensionBehaviour _bh,
               DimensionVariation _dv, StreamType _typ, StreamSetting _mod
               ) {
         stream = new SEStream(_s, _typ, _mod);
         dimension = new SEDimension(_sz, _bh, _tg, _dv);
         tc = _tc;
     }
     // Create command for end and append
     // SECommand(StreamID _s, StreamModif _sm, StreamConfig _cfg,
     //     DimensionSize _sz, DimensionOffset _o, DimensionStride _st,
     //     StreamType _type){
     //     stream = new SEStream(_s, _sm, _cfg, _type);
     //     dimension = new SEDimension(_sz, _o, _st);
     //     }
     ~SECommand(){
         // if (stream != nullptr) delete stream; delete dimension;
     };

     void set_description_end() { stream->setType(StreamType::end); }

     std::string to_string() {
         return csprintf("{Stream: %s, Dimension: %s}", stream->to_string(),
                         dimension->to_string());
     }

     SEDimension* get_dimension() { return dimension; }

     uint8_t get_width() { return stream->getWidth(); }

     uint8_t get_compressed_width() { return stream->getCompressedWidth(); }

     uint8_t getStreamID() { return stream->getID(); }

     bool isStart() { return stream->getType() == StreamType::start; }
     bool isEnd() { return stream->getType() == StreamType::end; }
     bool isAppend() { return stream->getType() == StreamType::append; }
     bool isStartEnd() { return stream->getType() == StreamType::simple; }

     bool isDimension() {
         return stream->getSetting() == StreamSetting::dimension;
     }
     bool isModifier() {
         return stream->getSetting() == StreamSetting::modifier;
     }

     bool isLast() {
         if (stream->getType() == StreamType::end ||
             stream->getType() == StreamType::simple)
             return true;
         else
             return false;
     }

     bool isLoad() { return stream->getMode() == StreamMode::load; }

     ThreadContext* get_tc() { return tc; }
};

typedef struct commandSortWrapper {
    commandSortWrapper() : complete(false), sn(0), sid(0) {
        cmd = SECommand();
    }
    SECommand cmd;
    bool complete;
    InstSeqNum sn;
    StreamID sid;
} CommandSortWrapper;

using SEStack = std::list<CommandSortWrapper>;

class DimensionObject{
    private:
        bool dimensionEnded = true;
        bool lowerEnded = false;
        int64_t counter;
        DimensionOffset offset;
        DimensionSize size;
        DimensionStride stride;
        DimensionOffset saved_offset = 0;
        uint8_t width;
        bool isHead;
        bool isEnd;
        bool isTerminated;
        uint8_t decrement;

    public:
     DimensionObject(SEDimension* dimension, uint8_t _width, bool head = false,
                     bool end = false)
         : counter(-1),
           offset(dimension->get_offset()),
           size(dimension->get_size()),
           stride(dimension->get_stride()),
           width(_width),
           isHead(head),
           isEnd(end),
           isTerminated(false) {
         decrement = isHead ? 2 : 1;
         saved_offset = isHead ? offset : offset * stride * width;
        }
        ~DimensionObject(){}

        bool advance(){
            lowerEnded = false;
            if (dimensionEnded)
                counter = -1;
            counter ++;
            if (counter > size - decrement) {
                if (!isHead) counter--;
                dimensionEnded = true;
            }
            else dimensionEnded = false;
            return dimensionEnded;
        }

        bool peek() {
            int64_t aux_counter = counter;
            bool aux_dimensionEnded = dimensionEnded;

            if (aux_dimensionEnded) aux_counter = -1;
            aux_counter++;
            if (aux_counter > size - decrement) {
                aux_dimensionEnded = true;
            } else
                aux_dimensionEnded = false;
            return aux_dimensionEnded;
        }

        int64_t get_counter() {return counter;}
        void set_counter(int64_t count) {counter = count;}

        DimensionStride get_size(){return size;}

        int64_t get_initial_offset(bool zero){
            if (zero){
                if (isHead){
                    return offset;
                }
                else {
                    return offset * stride * width;
                }
            }
            else {
                return saved_offset;
            }
        }
        int64_t get_cur_offset(){
            if (counter == -1) return get_initial_offset(true);

            if (isHead)
                return counter*stride*width+offset;
            else {
                if (has_ended()) return get_initial_offset(true);
                return (counter+offset)*width*stride;
            }
        }

        void set_offset(){
            if (dimensionEnded){
                saved_offset = get_initial_offset(true);
                return;
            }
            if (isHead || lowerEnded){
                saved_offset = get_cur_offset() + stride*width;
            } else {
                saved_offset = get_cur_offset();
            }
        }

        bool has_ended() { return dimensionEnded; }

        void lower_ended(){
            lowerEnded = true;
        }

        std::string
        to_string(){
            return csprintf("S(%d):O(%d):St(%d)", size, offset, stride);
        }

        bool hasBottomEnded() {
            bool res = isEnd && isTerminated;
            isTerminated = true;
            return res;
        }
};

typedef enum { Ended, Running, Configured, Clean, Stalled } SEIterationStatus;
typedef enum { BufferFull, DimensionSwap, End, NonCoallescing } StopReason;

struct SERequestInfo{
    SERequestInfo()
        : initial_offset(0),
          final_offset(0),
          width(0),
          status(SEIterationStatus::Clean),
          sequence_number(0),
          initial_paddr(0),
          sid(0),
          tc(nullptr),
          mode(StreamMode::load),
          stop_reason(StopReason::BufferFull){};
    DimensionOffset initial_offset;
    DimensionOffset final_offset;
    uint8_t width;
    uint8_t iterations;
    SEIterationStatus status;
    int64_t sequence_number;
    Addr initial_paddr;
    StreamID sid;
    ThreadContext * tc;
    StreamMode mode;
    StopReason stop_reason;
};

class SEIter: public SEList<DimensionObject> {
    private:
        std::function<bool(Addr, Addr)> pageFunction;
        StreamID sid;
        DimensionOffset initial_offset;
        uint8_t width;
        tnode * current_nd;
        tnode * current_dim;
        uint64_t elem_counter;
        int64_t sequence_number;
        int64_t end_ssid;
        Tick start_tick;
        SEIterationStatus status;
        uint64_t head_stride;
        SERequestInfo cur_request;
        SERequestInfo last_request;
        ThreadContext * tc;
        bool page_jump;
        Addr page_jump_vaddr;
        StreamMode mode;

    public:
     SEIter(SEStack* cmds, Tick _start_tick);
     SEIter();
     ~SEIter();

     template <typename F>
     void setCompareFunction(F f) {
         pageFunction = f;
     }

     DimensionOffset initial_offset_calculation(tnode* cursor,
                                                DimensionOffset offset,
                                                bool zero);

     DimensionOffset initial_offset_calculation(bool zero = false);

     DimensionOffset offset_calculation(tnode* cursor, DimensionOffset offset);

     DimensionOffset offset_calculation();

     void stats(SERequestInfo result, StopReason sres);

     SERequestInfo advance(uint16_t max_size);
     bool empty();

     uint8_t getWidth();

     uint8_t getCompressedWidth();

     bool translatePaddr(Addr* paddr);

     void setPaddr(Addr paddr, bool new_page = false, Addr vaddr = 0);

     SmartReturn ended();
     int64_t get_end_ssid();

     void stall();
     void resume();
     bool stalled();
     bool is_load();

     Tick time();

     const char* print_state();
};

using SEIterPtr = SEIter *;

#endif //__UVE_SE_UTILS_HH__
