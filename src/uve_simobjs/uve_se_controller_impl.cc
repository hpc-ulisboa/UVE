
#include "debug/UVESE.hh"
#include "uve_simobjs/uve_streaming_engine.hh"

SEcontroller::SEcontroller(UVEStreamingEngineParams *params,
                            UVEStreamingEngine *_parent)
{
    parent = _parent;
    memCore = &_parent->memCore;
}

Tick SEcontroller::recvCommand(SECommand cmd){
    //Parse command (switch case)
    //Handle request, return time
    DPRINTF(UVESE, "recvCommand %s\n", cmd.to_string());

    uint8_t sID = cmd.getStreamID();

    cmdQueue[sID].push(cmd);
    if(cmd.isLast()){
        SEStack * que = &cmdQueue[sID];
        SEIterPtr stream_iterator = new SEIter(que);
        //Send iterator to processing engine
        memCore->setIterator(sID, stream_iterator);
    }
    return 1;
}