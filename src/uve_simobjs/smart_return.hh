#ifndef __UVE_SE_SMARTRETURN_HH__
#define __UVE_SE_SMARTRETURN_HH__

#include <cassert>
#include <sstream>
#include <string>

class SmartReturn {
   private:
    typedef enum {
        END = 1,
        OK = 2,
        NOK = -1,
        CLEAR = 0,
        ERROR = -2
    } ReturnCode;
    ReturnCode status;
    void* data;
    bool data_used;
    SmartReturn(ReturnCode rt, void* _data) {
        status = rt;
        data = _data;
        if (_data == NULL) {
            data_used = true;
        } else {
            data_used = false;
        }
    }

   public:
    ~SmartReturn(){
        // if (!data_used) {
        //     // panic("Data in smart return not used!");
        // }
    };
    static SmartReturn ok(void* _data = NULL) {
        return SmartReturn(ReturnCode::OK, _data);
    }
    bool isOk() { return status == ReturnCode::OK; }
    static SmartReturn nok(void* _data = NULL) {
        return SmartReturn(ReturnCode::NOK, _data);
    }
    bool isNok() { return status == ReturnCode::NOK; }
    static SmartReturn end(void* _data = NULL) {
        return SmartReturn(ReturnCode::END, _data);
    }
    bool isEnd() { return status == ReturnCode::END; }
    static SmartReturn error(void* _data = NULL) {
        return SmartReturn(ReturnCode::ERROR, _data);
    }
    bool isError() { return status == ReturnCode::ERROR; }
    bool hasData() { return data != NULL; }
    void* getData() {
        data_used = true;
        return data;
    }
    static SmartReturn compare(bool comparisson, void* _data = NULL) {
        return comparisson ? SmartReturn::ok(_data) : SmartReturn::nok(_data);
    }

    inline bool isTrue() { return isOk(); }
    inline bool isFalse() { return isNok(); }

    SmartReturn AND(SmartReturn b) {
        if (isError() || b.isError()) return *this;
        if (isEnd() && b.isOk()) return *this;
        if (isOk() && b.isEnd()) return b;
        if (isNok() || b.isNok())
            return SmartReturn(NOK, b.hasData() ? b.getData() : getData());
        if (isOk() && b.isOk())
            return SmartReturn(OK, b.hasData() ? b.getData() : getData());
        return SmartReturn::error();
    };
    SmartReturn OR(SmartReturn b) {
        if (isError() || b.isError()) return *this;
        if (isEnd() && b.isOk()) return *this;
        if (isOk() && b.isEnd()) return b;
        if (isNok() && b.isNok())
            return SmartReturn(NOK, b.hasData() ? b.getData() : getData());
        if (isOk() || b.isOk())
            return SmartReturn(OK, b.hasData() ? b.getData() : getData());
        return SmartReturn::error();
    };

    void ASSERT() {
        if (isNok()) {
            panic("SmartReturn::ASSERT: NOK");
            return;
        }

        if (isError()) {
            panic("SmartReturn::ASSERT: Error");
            return;
        }
    }

    void NASSERT() {
        if (isOk()) {
            panic("SmartReturn::NASSERT: OK");
            return;
        }

        if (isError()) {
            panic("SmartReturn::NASSERT: Error");
            return;
        }
    }
};

#endif
