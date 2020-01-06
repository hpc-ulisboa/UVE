#include "uve_simobjs/uve_ff_load.hh"

UVEStoreFifo::UVEStoreFifo(UVEStreamingEngineParams *params)
    : SimObject(params),
      fifos(32),
      cacheLineSize(params->system->cacheLineSize()),
      confParams(params),
      fifo_depth(confParams->fifo_depth) {
    reservation_ssid = -1;
    for (int i = 0; i < fifos.size(); i++) {
        fifos[i] = new StreamFifo(confParams->width, confParams->fifo_depth,
                                  confParams->max_request_size, i);
    }
}

void
UVEStoreFifo::init() {
    SimObject::init();
}

bool
UVEStoreFifo::tick(CallbackInfo *info) {
    // Check if fifo is ready and answer true if it is
    int8_t k = -1;
    for (int i = 0; i < fifos.size(); i++) {
        if (fifos[i]->ready().isOk()) {
            info->psids[++k] = i;
        }
    }
    if (k != -1) {
        info->psids_size = k + 1;
        return true;
    }
    return false;
}

SmartReturn
UVEStoreFifo::getData(int sid) {
    // Check if fifo is ready and if so give data to the engine
    if (fifos[sid]->storeReady().isTrue()) {
        auto entry = fifos[sid]->storeGet();
        return SmartReturn::ok((void *)new CoreContainer(entry));
    } else {
        return SmartReturn::nok();
    }
}

void
UVEStoreFifo::synchronizeLists(StreamID sid) {
    fifos[sid]->synchronizeLists();
}

SmartReturn
UVEStoreFifo::reserve(StreamID sid, uint16_t *ssid) {
    // Reserve space in fifo. This reserve comes from the cpu in the rename
    SmartReturn result = fifos[sid]->full();
    if (result.isError() || result.isTrue()) {
        std::stringstream s;
        s << "Trying to reserve on fifo " << (int)sid
          << " with no space available" << result.isError()
            ? " Error: " + result.estr()
            : "";
        panic(s.str());
    }
    reservation_ssid++;
    fifos[sid]->insert(confParams->width, reservation_ssid, 0, false);
    *ssid = reservation_ssid;
    return SmartReturn::ok();
}

SmartReturn
UVEStoreFifo::insert_data(StreamID sid, uint32_t ssid, CoreContainer data) {
    // If not empty try and merge
    //  Not Succeded: Create new entry
    //      If full: Warn no more space
    //      Else: Create new entry
    fifos[sid]->empty().NASSERT();

    if (fifos[sid]->merge_data(ssid, data.raw_ptr<uint8_t>()).isOk()) {
        return SmartReturn::ok();
    }
    return SmartReturn::nok();
}

SmartReturn
UVEStoreFifo::full(StreamID sid) {
    return fifos[sid]->full();
}

SmartReturn
UVEStoreFifo::ready(StreamID sid) {
    return fifos[sid]->storeReady();
}

SmartReturn
UVEStoreFifo::squash(StreamID sid, uint16_t ssid) {
    return fifos[sid]->storeSquash(ssid);
}

SmartReturn
UVEStoreFifo::shouldSquash(StreamID sid) {
    return fifos[sid]->shouldSquash();
}

SmartReturn
UVEStoreFifo::commit(StreamID sid, uint16_t ssid) {
    return fifos[sid]->storeCommit(ssid);
}

SmartReturn
UVEStoreFifo::clear(StreamID sid) {
    delete fifos[sid];
    fifos[sid] = new StreamFifo(confParams->width, confParams->fifo_depth,
                                confParams->max_request_size, sid);
    return SmartReturn::ok();
}

SmartReturn
UVEStoreFifo::isFinished(StreamID sid) {
    // JMFIXME: Probably will not work (use start and end counter, keep state
    // of configuration start and end)
    return fifos[sid]->complete();
}

uint16_t
UVEStoreFifo::getAvailableSpace(StreamID sid) {
    // JMFIXME: Probably will not work (use start and end counter, keep state
    // of configuration start and end)
    return fifos[sid]->availableSpace();
}
