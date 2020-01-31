#ifndef __UVE_SE_UTILS_HH__
#define __UVE_SE_UTILS_HH__

#include <queue>

#include <boost/serialization/strong_typedef.hpp>

#include "base/cprintf.hh"
#include "cpu/thread_context.hh"
#include "debug/JMDEVEL.hh"
#include "sim/sim_object.hh"
#include "smart_return.hh"
#include "tree_utils.hh"

#define MaximumStreams 32
#define STOP_SIZE 512

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

template <typename T>
T mult_all(std::vector<T> * vec){
    uint64_t result = 1;
    for(auto&& x: *vec){
        result *= x;
    }
    return (T)result;
}

typedef uint64_t DimensionSize;
typedef uint64_t DimensionOffset;
typedef uint64_t DimensionStride;

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
        specification type;
    public:
        SEDimension(DimensionSize size, DimensionOffset offset,
            DimensionStride stride):
                size(size), offset(offset), stride(stride)
            {   type = specification::complete;}
        SEDimension(DimensionOffset offset):
                offset(offset)
            {   type = specification::init;}
        ~SEDimension() {}

        std::string
        to_string(){
            return csprintf("S(%d):O(%d):St(%d)", size, offset, stride);
        }

        DimensionSize get_size() { return size; }
        DimensionStride get_stride() { return stride; }
        DimensionOffset get_offset() { return offset; }

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

    public:
        SEStream(StreamID stream, StreamWidth width, StreamMode mode,
            StreamType type):
             width(width), mode(mode), type(type)
        {
            assert(stream < MaximumStreams);
            id = stream;
        }
        //Append and End dont now width or mode
        SEStream(StreamID stream, StreamType type):
             type(type)
        {
            assert(type == StreamType::append || type == StreamType::end);
            assert(stream < MaximumStreams);
            id = stream;
        }
        ~SEStream(){}

        std::string
        to_string(){
            return csprintf("ID(%d):T(%c):W(%c):%c",id, (char) type,
                (char)width, (char)mode);
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
        void setType(StreamType _type){
            if(type==start)
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
         return true;
         // JMTODO: Support modifiers
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
            else
                return (counter+offset)*width*stride;
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

struct SERequestInfo{
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
};

SERequestInfo static makeSERequestInfo() {
    SERequestInfo a;
    a.initial_offset = 0;
    a.final_offset = 0;
    a.width = 0;
    a.iterations = 0;
    a.status = SEIterationStatus::Clean;
    a.sequence_number = 0;
    a.initial_paddr = 0;
    a.sid = 0;
    a.tc = nullptr;
    a.mode = StreamMode::load;
    return a;
}

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
        typedef enum {
            BufferFull,
            DimensionSwap,
            End,
            NonCoallescing
        }StopReason;

    public:
     SEIter(SEStack* cmds, Tick _start_tick)
         : SEList(),
           elem_counter(0),
           sequence_number(-1),
           end_ssid(-1),
           start_tick(_start_tick) {
         // JMTODO: Create iterator from cmds
         // Validate all commands
         // Create iteration tree
         // Empty Stack
         status = SEIterationStatus::Configured;
         DPRINTF(JMDEVEL, "SEIter constructor\n");

         SECommand cmd = cmds->front().cmd;
         width = cmd.get_width();
         head_stride = cmd.get_dimension()->get_stride();
         tc = cmd.get_tc();
         sid = cmd.getStreamID();
         mode = cmd.isLoad() ? StreamMode::load : StreamMode::store;

         insert_dim(new DimensionObject(cmd.get_dimension(), width, true));
         cmds->pop_front();

         DimensionObject* dimobj;
         while (!cmds->empty()) {
             cmd = cmds->front().cmd;
             if (cmds->size() == 1) {
                 dimobj = new DimensionObject(cmd.get_dimension(), width,
                                              false, true);
             } else {
                 dimobj = new DimensionObject(cmd.get_dimension(), width);
             }

             DPRINTF(JMDEVEL, "Inserting command: %s\n", cmd.to_string());

             if (cmd.isDimension())
                 insert_dim(dimobj);
             else
                 insert_mod(dimobj);
             cmds->pop_front();
         }
         DPRINTF(JMDEVEL, "Tree: \n%s\n", this->to_string());
         initial_offset = initial_offset_calculation(true);
         current_dim = get_end();
        }
        SEIter() {
            status = SEIterationStatus::Clean;
            pageFunction = nullptr;
            sid = 0;
            initial_offset = 0;
            width = 0;
            current_nd = nullptr;
            current_dim = nullptr;
            elem_counter = 0;
            sequence_number = 0;
            end_ssid = 0;
            start_tick = 0;
            head_stride = 0;
            cur_request = makeSERequestInfo();
            last_request = makeSERequestInfo();
            tc = nullptr;
            page_jump = false;
            page_jump_vaddr = 0;
            mode = StreamMode::load;
        }
        ~SEIter(){}

        template <typename F>
        void setCompareFunction(F f){
            pageFunction = f;
        }

        DimensionOffset initial_offset_calculation(tnode * cursor,
                            DimensionOffset offset, bool zero){

            if (cursor->next != nullptr){
                offset = initial_offset_calculation(cursor->next,
                                                    offset, zero);
            }
            return offset + cursor->content->get_initial_offset(zero);
        }

        DimensionOffset initial_offset_calculation(bool zero = false){
            tnode * cursor = get_head();
            DimensionOffset offset = 0;
            if(cursor->next != nullptr)
                offset = initial_offset_calculation(cursor->next,offset, zero);
            offset += cursor->content->get_initial_offset(zero);
            return offset;
        }

        DimensionOffset offset_calculation(tnode * cursor,
                                        DimensionOffset offset){
            if (cursor->next != nullptr){
                offset = offset_calculation(cursor->next, offset);
            }
            cursor->content->set_offset();
            return offset + cursor->content->get_cur_offset();
        }

        DimensionOffset offset_calculation(){
            tnode * cursor = get_head();
            DimensionOffset offset = 0;
            if(cursor->next != nullptr)
                offset = offset_calculation(cursor->next,offset);
            cursor->content->set_offset();
            offset += cursor->content->get_cur_offset();
            return offset;
        }

        void stats(SERequestInfo result, StopReason sres){
            //Create results with this
        };

        SERequestInfo advance(uint16_t max_size) {
            //Iterate until request is generated
            assert(status != SEIterationStatus::Ended &&
                   status != SEIterationStatus::Clean &&
                   status != SEIterationStatus::Stalled);

            SERequestInfo request;
            bool result = false;
            status = SEIterationStatus::Running;
            StopReason sres = StopReason::BufferFull;

            while (elem_counter < max_size / (width * 8)) {
                assert(current_dim != nullptr);

                result = current_dim->content->advance();
                if(result == true){//Dimension Change -> Issue request
                    sres = StopReason::DimensionSwap;
                    if(current_dim->next == nullptr){
                        status = SEIterationStatus::Ended;
                        sres = StopReason::End;
                        break;
                    }
                    current_dim = current_dim->next;
                    current_dim->content->lower_ended();

                    // Do a peek to assure it is not the last iteration
                    auto aux_dim = current_dim;
                    while (aux_dim != nullptr) {
                        if (aux_dim->content->peek()) {
                            aux_dim = aux_dim->next;
                        } else
                            break;
                    };
                    if (aux_dim == nullptr) {
                        status = SEIterationStatus::Ended;
                        sres = StopReason::End;
                    }
                    break;
                }
                else {
                    if(current_dim != get_head()) {
                        //We are not in the lowest dimension
                        // Need to go back to the lower dimension
                        current_dim = current_dim->prev;
                        continue;
                    }
                    else if(head_stride != 1){
                        sres = StopReason::NonCoallescing;
                        break;
                    }
                    else elem_counter ++;
                }
            }
            elem_counter = 0;
            request.mode = mode;
            request.initial_offset = initial_offset_calculation();
            request.final_offset = offset_calculation() + width;
            request.iterations = 128;
            request.status = status;
            request.width = width;
            request.tc = tc;
            request.sid = sid;
            request.sequence_number = ++sequence_number;
            if (status == SEIterationStatus::Ended){
                end_ssid = sequence_number - 1;
            }
            initial_offset = request.final_offset;
            last_request = cur_request;
            cur_request = request;
            stats(request, sres);
            return request;
        }

        bool empty(){ return (status == SEIterationStatus::Clean ||
                              status == SEIterationStatus::Ended ) ?
                        true : false;}

        uint8_t getWidth(){return width;}

        uint8_t getCompressedWidth() {
            uint8_t _width = 0;
            switch (width) {
                case 1:
                    _width = 0;
                    break;
                case 2:
                    _width = 1;
                    break;
                case 4:
                    _width = 2;
                    break;
                case 8:
                    _width = 3;
                    break;
            }
            return _width;
        }

        // returns true if it could not translate
        // paddr is passed by referenced, contains the translation result
        // if it succeds (return is true)
        bool translatePaddr(Addr * paddr){
            //First request must always be translated by TLB
            if (sequence_number == 0) {
                last_request = cur_request;
                return false;
            }

            //Check if the page is mantained between last and current request
            if (pageFunction(cur_request.initial_offset,
                             cur_request.final_offset) &&
                pageFunction(last_request.initial_offset,
                             cur_request.initial_offset) &&
                pageFunction(last_request.initial_offset,
                             last_request.final_offset)) {
                *paddr =
                    last_request.initial_paddr +
                    (cur_request.initial_offset - last_request.initial_offset);
                return true;

                // If the page is not mantained see if the current one is and
                // check that the last translation targets a new page
            } else if (pageFunction(cur_request.initial_offset,
                                    cur_request.final_offset) &&
                       page_jump) {
                *paddr = last_request.initial_paddr +
                         (cur_request.initial_offset - page_jump_vaddr);
                return true;
            } else {
                // In this case it is not possible to infer the paddr
                // Data is in new page
                return false;
            }
        }

        void setPaddr(Addr paddr, bool new_page = false, Addr vaddr = 0) {
            cur_request.initial_paddr = paddr;
            page_jump_vaddr = vaddr;
            page_jump = new_page;
        }

        SmartReturn ended() {
            return SmartReturn::compare(status == SEIterationStatus::Ended);
        }
        int64_t get_end_ssid() { return end_ssid; }

        void stall() { status = SEIterationStatus::Stalled; }
        void resume() { status = SEIterationStatus::Running; }
        bool stalled() { return status == SEIterationStatus::Stalled; }
        bool is_load() { return mode == StreamMode::load; }

        Tick time() { return start_tick; }
};

using SEIterPtr = SEIter *;

#endif //__UVE_SE_UTILS_HH__
