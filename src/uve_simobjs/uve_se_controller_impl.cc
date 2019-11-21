
#include "debug/UVESE.hh"
#include "uve_simobjs/uve_streaming_engine.hh"

SEcontroller::SEcontroller(UVEStreamingEngineParams *params,
                            UVEStreamingEngine *_parent)
{
    parent = _parent;
    memCore = &_parent->memCore;
}

SmartReturn
SEcontroller::recvCommand(SECommand cmd) {
    //Parse command (switch case)
    //Handle request, return time
    DPRINTF(UVESE, "recvCommand %s\n", cmd.to_string());

    uint8_t sID = cmd.getStreamID();

    cmdQueue[sID].push(cmd);
    if(cmd.isLast()){
        SEStack * que = &cmdQueue[sID];
        SEIterPtr stream_iterator = new SEIter(que);
        //Send iterator to processing engine
        if (!memCore->setIterator(sID, stream_iterator)) {
            DPRINTF(UVESE, "Starting Stream: Stream %d is not available.\n",
                    sID);
            delete stream_iterator;
        }
    }
    return SmartReturn::ok();
}