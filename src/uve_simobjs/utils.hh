#ifndef __UVE_SE_UTILS_HH__
#define __UVE_SE_UTILS_HH__

#include "sim/sim_object.hh"
#include "base/cprintf.hh"
#include "tree_utils.hh"
#include "debug/JMDEVEL.hh"
#include <queue>

#define MaximumStreams 32
#define PACKET_SIZE 1024



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
        StreamMode getMode(){
            return mode;
        }
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
    public:
        //Create command from stream and dimension objects
        SECommand(SEStream * _stream, SEDimension * _dimension) {
            stream = _stream;
            dimension = _dimension;
        }
        //Create command for simple stream
        SECommand(StreamID _s, StreamWidth _w, StreamMode _m,
            DimensionOffset _o, DimensionSize _sz, DimensionStride _st, StreamType type = StreamType::simple){
            stream = new SEStream(_s, _w, _m, type);
            dimension = new SEDimension(_sz, _o, _st);
        }
        //Create command for init
        SECommand(StreamID _s, StreamWidth _w, StreamMode _m,
            DimensionOffset _o){
                stream = new SEStream(_s, _w, _m, StreamType::start);
                dimension = new SEDimension(_o);
            }
        //Create command for simple append and end
        SECommand(StreamID _s,
            DimensionOffset _o, DimensionSize _sz, DimensionStride _st, StreamType type = StreamType::append){
            stream = new SEStream(_s, type);
            dimension = new SEDimension(_sz, _o, _st);
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

        uint8_t getStreamID(){
            return stream->getID();
        }

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
};

using SEStack = std::queue<SECommand>;

class DimensionObject{
    private:
        bool dimensionEnded = true;
        int64_t counter;
        DimensionOffset offset;
        DimensionSize size;
        DimensionStride stride;
        uint8_t width;
        bool isHead;
        uint8_t decrement;

    public:
        DimensionObject(SEDimension * dimension, uint8_t _width, bool head = false):
            counter(-1), offset(dimension->get_offset()),
            size(dimension->get_size()), stride(dimension->get_stride()),
            width(_width), isHead(head)
        {decrement = isHead ? 1 : 0;}
        ~DimensionObject(){}

        bool advance(){
            if(dimensionEnded == true) counter = -1;
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

        int64_t get_initial_offset(){}
        int64_t get_cur_offset(){}

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
};

class SEIter: public SEList<DimensionObject> {
    private:
        DimensionOffset initial_offset;
        uint8_t width;
        tnode * current_nd;
        tnode * current_dim;
        uint8_t elem_counter;
        uint8_t sequence_number;
        SEIterationStatus status;
        uint64_t head_stride;
        typedef enum {
            BufferFull,
            DimensionSwap,
            End,
            NonCoallescing
        }StopReason;

    public:
        SEIter(SEStack * cmds): SEList(),
        elem_counter(0), sequence_number(0){
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

            while(!cmds->empty()){
                cmd = cmds->front();
                DimensionObject * dimobj =
                    new DimensionObject(cmd.get_dimension(), width, first_iteration);
                first_iteration = false;

                DPRINTF(JMDEVEL, "Inserting command: %s\n", cmd.to_string());

                if(cmd.isDimension())
                    insert_dim(dimobj);
                else
                    insert_mod(dimobj);
                cmds->pop();
            }
            DPRINTF(JMDEVEL, "Tree: \n%s\n",this->to_string());
            initial_offset = initial_offset_calculation();
            current_dim = get_end();
        }
        SEIter() {
            status = SEIterationStatus::Clean;
        }
        ~SEIter(){}

        DimensionOffset initial_offset_calculation(tnode * cursor, DimensionOffset offset){
            if(cursor->next != nullptr){
                offset = initial_offset_calculation(cursor->next, offset);
            }
            return offset + cursor->content->get_initial_offset();;
        }

        DimensionOffset initial_offset_calculation(){
            tnode * cursor = get_head();
            DimensionOffset offset = 0;
            if(cursor->next != nullptr)
                offset = initial_offset_calculation(cursor->next,offset);
            offset += cursor->content->get_initial_offset();
            return offset;
        }

        DimensionOffset offset_calculation(tnode * cursor, DimensionOffset offset){
            if(cursor->next != nullptr){
                offset = offset_calculation(cursor->next, offset);
            }
            return offset + cursor->content->get_cur_offset();
        }

        DimensionOffset offset_calculation(){
            tnode * cursor = get_head();
            DimensionOffset offset = 0;
            if(cursor->next != nullptr)
                offset = offset_calculation(cursor->next,offset);
            offset += cursor->content->get_cur_offset();
            return offset;
        }


        SERequestInfo advance(){
            //Iterate until request is generated
            assert(status != SEIterationStatus::Ended &&
                    status != SEIterationStatus::Clean );

            SERequestInfo request;
            bool result = false;
            status = SEIterationStatus::Running;
            StopReason sres = StopReason::BufferFull;

            while(elem_counter < PACKET_SIZE / width){
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
            if(sres == StopReason::BufferFull){
                elem_counter = 0;
                request.final_offset = offset_calculation() + width;
                request.initial_offset = initial_offset;
                request.iterations = 128;
                request.status = status;
                request.sequence_number = ++sequence_number;
                request.width = width;
                initial_offset = request.final_offset;
            }
            else if(sres == StopReason::DimensionSwap ||
                    sres == StopReason::NonCoallescing){
                elem_counter = 0;
                request.final_offset = offset_calculation() + width;
                request.initial_offset = initial_offset_calculation();
                request.iterations = 128;
                request.status = status;
                request.sequence_number = ++sequence_number;
                request.width = width;
                initial_offset = request.final_offset;
            }
            return request;
        }

        bool empty(){ return (status == SEIterationStatus::Clean ||
                              status == SEIterationStatus::Ended ) ?
                        true : false;}

};

using SEIterPtr = SEIter *;

#endif //__UVE_SE_UTILS_HH__