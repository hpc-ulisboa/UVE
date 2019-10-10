#ifndef __UVE_SE_UTILS_HH__
#define __UVE_SE_UTILS_HH__

#include <queue>

#include "base/cprintf.hh"
#include "cpu/thread_context.hh"
#include "debug/JMDEVEL.hh"
#include "sim/sim_object.hh"
#include "tree_utils.hh"

#define MaximumStreams 32
#define STOP_SIZE 1024


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
typedef uint8_t StreamID;

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
        SEStream * stream;
        SEDimension * dimension;
        ThreadContext * tc;
    public:
        //Create command from stream and dimension objects
        SECommand(ThreadContext * _tc, SEStream * _stream,
                    SEDimension * _dimension) {
            stream = _stream;
            dimension = _dimension;
            tc = _tc;
        }
        //Create command for simple stream
        SECommand(ThreadContext * _tc, StreamID _s, StreamWidth _w,
                StreamMode _m, DimensionOffset _o, DimensionSize _sz,
                DimensionStride _st, StreamType type = StreamType::simple){
            stream = new SEStream(_s, _w, _m, type);
            dimension = new SEDimension(_sz, _o, _st);
            tc = _tc;
        }
        //Create command for init
        SECommand(ThreadContext * _tc, StreamID _s, StreamWidth _w,
                StreamMode _m, DimensionOffset _o){
                stream = new SEStream(_s, _w, _m, StreamType::start);
                dimension = new SEDimension(_o);
                tc = _tc;
            }
        //Create command for simple append and end
        SECommand(ThreadContext * _tc, StreamID _s, DimensionOffset _o,
                DimensionSize _sz, DimensionStride _st,
                StreamType type = StreamType::append){
            stream = new SEStream(_s, type);
            dimension = new SEDimension(_sz, _o, _st);
            tc = _tc;
        }
        //Create command for end and append
        // SECommand(StreamID _s, StreamModif _sm, StreamConfig _cfg,
        //     DimensionSize _sz, DimensionOffset _o, DimensionStride _st,
        //     StreamType _type){
        //     stream = new SEStream(_s, _sm, _cfg, _type);
        //     dimension = new SEDimension(_sz, _o, _st);
        //     }
        ~SECommand(){
            // if(stream != nullptr) delete stream; delete dimension;
        };

        void
        set_description_end(){
            stream->setType(StreamType::end);
        }

        std::string
        to_string(){
            return csprintf("{Stream: %s, Dimension: %s}",stream->to_string(),
                dimension->to_string());
        }

        SEDimension *
        get_dimension(){
            return dimension;
        }

        uint8_t
        get_width(){
            return stream->getWidth();
        }

        uint8_t get_compressed_width() { return stream->getCompressedWidth(); }

        uint8_t getStreamID() { return stream->getID(); }

        bool isDimension(){
            return true;
            //JMTODO: Support modifiers
        }

        bool isLast(){
            if(stream->getType() == StreamType::end ||
                stream->getType() == StreamType::simple )
                return true;
            else return false;
        }

        ThreadContext * get_tc(){return tc;}
};

using SEStack = std::queue<SECommand>;

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
        uint8_t decrement;

    public:
        DimensionObject(SEDimension * dimension, uint8_t _width,
                    bool head = false):
            counter(-1), offset(dimension->get_offset()),
            size(dimension->get_size()), stride(dimension->get_stride()),
            width(_width), isHead(head)
        {
            decrement = isHead ? 1 : 0;
            saved_offset = isHead ? offset : offset*stride*width;
        }
        ~DimensionObject(){}

        bool advance(){
            lowerEnded = false;
            if (dimensionEnded)
                counter = -1;
            counter ++;
            if(counter >= size - decrement){
                dimensionEnded = true;
            }
            else dimensionEnded = false;
            return dimensionEnded;
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
};

typedef enum {
    Ended,
    Running,
    Configured,
    Clean
} SEIterationStatus;

struct SERequestInfo{
    DimensionOffset initial_offset;
    DimensionOffset final_offset;
    uint8_t width;
    uint8_t iterations;
    SEIterationStatus status;
    uint8_t sequence_number;
    Addr initial_paddr;
    StreamID sid;
    ThreadContext * tc;
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
        int16_t sequence_number;
        int16_t end_ssid;
        SEIterationStatus status;
        uint64_t head_stride;
        SERequestInfo cur_request;
        SERequestInfo last_request;
        ThreadContext * tc;
        typedef enum {
            BufferFull,
            DimensionSwap,
            End,
            NonCoallescing
        }StopReason;

    public:
        SEIter(SEStack * cmds):
        SEList(),
        elem_counter(0), sequence_number(-1), end_ssid(-1){
            //JMTODO: Create iterator from cmds
            //Validate all commands
            //Create iteration tree
            //Empty Stack
            bool first_iteration = true;
            status = SEIterationStatus::Configured;
            DPRINTF(JMDEVEL, "SEIter constructor\n");

            SECommand cmd = cmds->front();
            width = cmd.get_width();
            head_stride = cmd.get_dimension()->get_stride();
            tc = cmd.get_tc();
            sid = cmd.getStreamID();

            while(!cmds->empty()){
                cmd = cmds->front();
                DimensionObject * dimobj =
                    new DimensionObject(cmd.get_dimension(), width,
                                        first_iteration);
                first_iteration = false;

                DPRINTF(JMDEVEL, "Inserting command: %s\n", cmd.to_string());

                if(cmd.isDimension())
                    insert_dim(dimobj);
                else
                    insert_mod(dimobj);
                cmds->pop();
            }
            DPRINTF(JMDEVEL, "Tree: \n%s\n",this->to_string());
            initial_offset = initial_offset_calculation(true);
            current_dim = get_end();
        }
        SEIter() {
            status = SEIterationStatus::Clean;
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

        SERequestInfo advance(){
            //Iterate until request is generated
            assert(status != SEIterationStatus::Ended &&
                    status != SEIterationStatus::Clean );

            SERequestInfo request;
            bool result = false;
            status = SEIterationStatus::Running;
            StopReason sres = StopReason::BufferFull;

            while (elem_counter < STOP_SIZE / width){
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
                    break;
                }
                else {
                    if(current_dim != get_head()) {
                        //We are not in the lowest dimension
                        // Need to go back to the lower dimension
                        current_dim = current_dim->prev;
                    }
                    else if(head_stride != 1){
                        sres = StopReason::NonCoallescing;
                        break;
                    }
                    else elem_counter ++;
                }
            }
            elem_counter = 0;
            request.initial_offset = initial_offset_calculation();
            request.final_offset = offset_calculation() + width;
            request.iterations = 128;
            request.status = status;
            request.width = width;
            request.tc = tc;
            request.sid = sid;
            request.sequence_number = ++sequence_number;
            if (status == SEIterationStatus::Ended){
                end_ssid = sequence_number;
            }
            initial_offset = request.final_offset;
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
            if ( pageFunction(last_request.initial_offset,
                             last_request.final_offset) &&
                pageFunction(cur_request.initial_offset,
                             cur_request.final_offset) &&
                pageFunction(last_request.initial_offset,
                             cur_request.initial_offset) )
            {
                *paddr = last_request.initial_paddr +
                        (cur_request.initial_offset -
                        last_request.initial_offset);
                return true;
            }
            else {
                // In this case it is not possible to infer the paddr
                // Data is in new page
                return false;
            }
        }

        void setPaddr(Addr paddr){ last_request.initial_paddr = paddr;}

        bool ended(){return status == SEIterationStatus::Ended;}
        uint8_t get_end_ssid(){return end_ssid;}

};

using SEIterPtr = SEIter *;

#endif //__UVE_SE_UTILS_HH__
