
#include "debug/UVESE.hh"
#include "uve_simobjs/uve_streaming_engine.hh"

void SEprocessing::Tick(){
    // DPRINTF(JMDEVEL, "SEprocessing::Tick()\n");

    pID++;
    if (pID > 31) pID = 0;
    uint8_t auxID = pID + 1;
    if (auxID > 31) auxID = 0;

    while(iterQueue[auxID]->empty()){
        //One passage determines if no stream is configured
        if(auxID == pID){
            DPRINTF(JMDEVEL, "Blanck Tick\n");
            return;
        }
        auxID++;
        if (auxID > 31) auxID = 0;
    }

    DPRINTF(JMDEVEL, "Settled on auxID:%d\n", auxID);
    pID = auxID;
    SERequestInfo request_info = iterQueue[pID]->advance();
    if(request_info.status != SEIterationStatus::Ended)
        DPRINTF(UVESE,"Memory Request[%d]: [%d->%d] w(%d)\n", request_info.sequence_number,
        request_info.initial_offset, request_info.final_offset,
        request_info.width );
    else DPRINTF(UVESE,"Iteration on stream %d ended!\n", pID);
    return;
    //Create and send request to memory
}