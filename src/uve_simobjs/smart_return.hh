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
    std::string error_str;
    SmartReturn(ReturnCode rt, void* _data, std::string _str = std::string()) {
        status = rt;
        data = _data;
        error_str = _str;
    }

   public:
    ~SmartReturn(){};
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
    static SmartReturn error(std::string _str, void* _data = NULL) {
        return SmartReturn(ReturnCode::ERROR, _data, _str);
    }
    bool isError() { return status == ReturnCode::ERROR; }
    bool hasData() { return data != NULL; }
    void* getData() { return data; }
    static SmartReturn compare(bool comparisson, void* _data = NULL) {
        return comparisson ? SmartReturn::ok(_data) : SmartReturn::nok(_data);
    }

    inline bool isTrue() { return isOk(); }
    inline bool isFalse() { return isNok(); }

    std::string str() {
        switch (status) {
            case END:
                return std::string("END");
            case OK:
                return std::string("OK");
            case NOK:
                return std::string("NOK");
            case ERROR:
                return std::string("ERROR:[") + estr() + std::string("]");
            default:
                std::stringstream s;
                s << "Unsuported Result Status(" << status << ")";
                return s.str();
        }
    }

    std::string estr() { return error_str; }

    SmartReturn AND(SmartReturn b) {
        if (isError() || b.isError()) return *this;
        if (isEnd() && b.isOk()) return *this;
        if (isOk() && b.isEnd()) return b;
        if (isNok() || b.isNok())
            return SmartReturn(NOK, b.hasData() ? b.getData() : getData());
        if (isOk() && b.isOk())
            return SmartReturn(OK, b.hasData() ? b.getData() : getData());
        return SmartReturn::error("SmartReturn::AND Condition Failed");
    };
    SmartReturn OR(SmartReturn b) {
        if (isError() || b.isError()) return *this;
        if (isEnd() && b.isOk()) return *this;
        if (isOk() && b.isEnd()) return b;
        if (isNok() && b.isNok())
            return SmartReturn(NOK, b.hasData() ? b.getData() : getData());
        if (isOk() || b.isOk())
            return SmartReturn(OK, b.hasData() ? b.getData() : getData());
        return SmartReturn::error("SmartReturn::OR Condition Failed");
    };

    void ASSERT() {
        if (isNok()) {
            panic("SmartReturn::ASSERT: NOK");
            return;
        }

        if (isError()) {
            panic("SmartReturn::ASSERT: Error " + estr());
            return;
        }
    }

    void NASSERT() {
        if (isOk()) {
            panic("SmartReturn::NASSERT: OK");
            return;
        }

        if (isError()) {
            panic("SmartReturn::NASSERT: Error " + estr());
            return;
        }
    }
};

#endif
