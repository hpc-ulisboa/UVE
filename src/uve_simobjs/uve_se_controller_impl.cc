
#include "debug/UVESE.hh"
#include "uve_simobjs/uve_streaming_engine.hh"

SEcontroller::SEcontroller(UVEStreamingEngineParams *params,
                            UVEStreamingEngine *_parent)
{
    parent = _parent;
    memCore = &_parent->memCore;
}

SmartReturn
SEcontroller::recvCommand(SECommand cmd, InstSeqNum sn) {
    //Parse command (switch case)
    //Handle request, return time
    DPRINTF(UVESE, "recvCommand %s, sn %d\n", cmd.to_string(), sn);

    uint8_t sID = cmd.getStreamID();

    // Add command to list
    fill_cmd(sID, sn, cmd).ASSERT();

    if (cmds_ready(sID).isOk()) {
        auto* que = &cmdQueue[sID];
        que->sort([](CommandSortWrapper& a, CommandSortWrapper& b) {
            return a.sn < b.sn;
        });
        if (validate_cmds(sID).isNok()) return SmartReturn::nok();
        SEIterPtr stream_iterator = new SEIter(que, curTick());
        //Send iterator to processing engine
        if (!memCore->setIterator(sID, stream_iterator)) {
            DPRINTF(UVESE, "Starting Stream: Stream %d is not available.\n",
                    sID);
            delete stream_iterator;
        } else {
            parent->numStreamConfig[sID]++;
        }
    }
    return SmartReturn::ok();
}

SmartReturn
SEcontroller::addCmd(StreamID sid, InstSeqNum sn) {
    SEStack& cmds_list = cmdQueue[sid];
    // Insert
    CommandSortWrapper csw;
    csw.sn = sn;
    csw.sid = sid;
    cmds_list.push_front(csw);
    DPRINTF(UVESE, "queueCommand sid %d, sn %d\n", sid, sn);
    return SmartReturn::ok();
}

SmartReturn
SEcontroller::squashCmd(StreamID sid, InstSeqNum sn) {
    SEStack& cmds_list = cmdQueue[sid];
    // Insert
    auto iter = cmds_list.begin();
    while (iter != cmds_list.end()) {
        if (iter->sid == sid && iter->sn == sn) {
            cmds_list.erase(iter);
            return SmartReturn::ok();
        }
        iter++;
    }
    return SmartReturn::nok();
}

SmartReturn
SEcontroller::cmds_ready(StreamID sid) {
    SEStack cmds_list = cmdQueue[sid];
    auto iter = cmds_list.begin();
    while (iter != cmds_list.end()) {
        if (!iter->complete) return SmartReturn::nok();
        iter++;
    }
    return SmartReturn::ok();
}

SmartReturn
SEcontroller::fill_cmd(StreamID sid, InstSeqNum sn, SECommand cmd) {
    SEStack& cmds_list = cmdQueue[sid];
    auto iter = cmds_list.begin();
    while (iter != cmds_list.end()) {
        if (iter->sid == sid && iter->sn == sn) {
            iter->cmd = cmd;
            iter->complete = true;
            return SmartReturn::ok();
        }
        iter++;
    }
    return SmartReturn::nok();
}

SmartReturn
SEcontroller::validate_cmds(StreamID sid) {
    bool has_start = false, has_end = false;
    SEStack& cmds_list = cmdQueue[sid];
    auto iter = cmds_list.begin();
    while (iter != cmds_list.end()) {
        if (iter->sid != sid) return SmartReturn::nok();
        if (iter->cmd.isStartEnd()) {
            if (has_start || has_end) return SmartReturn::nok();
            has_start = true;
            has_end = true;
        } else if (iter->cmd.isStart()) {
            if (has_start) return SmartReturn::nok();
            has_start = true;
        } else if (iter->cmd.isEnd()) {
            if (has_end) return SmartReturn::nok();
            has_end = true;
        } else if (!iter->cmd.isAppend()) {
            return SmartReturn::nok();
        }

        iter++;
    }

    if (has_start && has_end)
        return SmartReturn::ok();
    else
        return SmartReturn::nok();
}