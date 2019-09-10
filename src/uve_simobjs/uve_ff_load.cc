#include "uve_simobjs/uve_ff_load.hh"

UVELoadFifo::UVELoadFifo(UVEStreamingEngineParams *params) :
  SimObject(params), fifos()
{
    fifos.fill(StreamFifo());
}

void
UVELoadFifo::init(){
    SimObject::init();
}

void
UVELoadFifo::tick(){

}

bool
UVELoadFifo::insert(StreamID sid, CoreContainer data, uint8_t size){
    //If empty put
    //If not empty try and merge
    //  Not Succeded: Create new entry
    //      If full: Warn no more space
    //      Else: Create new entry

    FifoEntry entry = FifoEntry(data, size);
    if (fifos[sid].empty()){
        fifos[sid].insert(entry);
    }
    else {
        if (!fifos[sid].merge(entry)){
            if (fifos[sid].full()){return false;}
            else {
                fifos[sid].insert(entry);
            }
        }
    }
    return true;
}

SmartContainer
UVELoadFifo::fetch(StreamID sid){
    if (fifos[sid].ready())
        return to_smart(fifos[sid].get());
}

SmartContainer
UVELoadFifo::to_smart(FifoEntry entry){
    SmartContainer ret;
    ret.first = *(&(entry.as<uint8_t>()));
    ret.second = entry.getEnable();
    return ret;
}

void
StreamFifo::insert(FifoEntry entry){

}

bool
StreamFifo::merge(FifoEntry entry){

}

FifoEntry
StreamFifo::get(){

}

bool
StreamFifo::ready(){

}

bool
StreamFifo::full(){

}