
#include "arch/riscv/insts/uve_utils.hh"

#include <sstream>
#include <string>

#include "arch/riscv/utility.hh"

using namespace std;

namespace RiscvISA{

    template <typename Ret>
    Ret convertFP(float& val){return (Ret) val;}
    template <typename Ret>
    Ret convertFP(double& val){return (Ret) val;}

    template <typename Ret>
    Ret convertUInt(uint8_t& val){return ((Ret&) val) & 0xff;}
    template <typename Ret>
    Ret convertUInt(uint16_t& val){return ((Ret&) val) & 0xffff;}
    template <typename Ret>
    Ret convertUInt(uint32_t& val){return ((Ret&) val) & 0xffffffff;}
    template <typename Ret>
    Ret convertUInt(uint64_t& val){return ((Ret&) val);}

    template <typename Ret>
    Ret convertInt(int8_t& val){
        auto aux = ((Ret&) val) & 0xff;
        if ((aux >> 7) & 1)
            aux |= ~(0ULL) << 8;
        return aux;
    }
    template <typename Ret>
    Ret convertInt(int16_t& val){
        auto aux = ((Ret&) val) & 0xffff;
        if ((aux >> 15) & 1)
            aux |= ~(0ULL) << 16;
        return aux;
    }
    template <typename Ret>
    Ret convertInt(int32_t& val){
        auto aux = ((Ret&) val) & 0xffffffff;
        if ((aux >> 31) & 1)
            aux |= ~(0ULL) << 32;
        return aux;
    }
    template <typename Ret>
    Ret convertInt(int64_t& val){
        return ((Ret&) val);
    }

    bool check_equal_src_widths(size_t width1,size_t width2){
        //JMTODO: Use assert in the future
        if (width1!=width2){
            DPRINTF(UVEUtils,"Src widths differ..(%d)!=(%d)\n",
            width1, width2);
            return false;
        }
        return true;
    }

    uint16_t get_dest_valid_index(uint16_t a, uint16_t b){
        return uveMin(a, b);
    }

    uint16_t get_dest_valid_index(uint16_t a, uint16_t b, uint16_t c){
        return uveMin(uveMin(a, b), c);
    }

    size_t get_vector_width(ExecContext *xc, uint8_t reg ){
        return RiscvStaticInst::getUveVecType(xc->tcBase(),
            reg);
    }

    size_t get_predicate_vector_width(ExecContext *xc, uint8_t reg ){
        return RiscvStaticInst::getUvePVecType(xc->tcBase(),
            reg);
    }


    template <typename Ret>
    Ret uveMax(Ret val1, Ret val2){
        return (val1 > val2 ? val1 : val2);
    }

    template <typename Ret>
    Ret uveMin(Ret val1, Ret val2){
        return (val1 < val2 ? val1 : val2);
    }

    template <typename Ret>
    bool uveEGT(Ret val1, Ret val2){
        return (val1 >= val2 ? true : false);
    }

    template <typename Ret>
    bool uveEQ(Ret val1, Ret val2){
        return (val1 == val2 ? true : false);
    }

    template <typename Ret>
    bool uveLT(Ret val1, Ret val2){
        return (val1 == val2 ? true : false);
    }


    template <typename Ret>
    Ret castBitsToRetType(uint64_t arg){
        union
        {
            Ret src;
            uint64_t bits;
        } val;
        val.bits = arg;
        return val.src;
    }

    template <typename T>
    uint64_t typeCastToBits(T arg)
    {
        union
        {
            T src;
            uint64_t bits;
        } val;
        val.src = arg;
        return val.bits;
    }


    // JMNOTE: Template Explicit Initialization

    template
    float convertFP(float& val);
    template
    double convertFP(float& val);
    template
    float convertFP(double& val);
    template
    double convertFP(double& val);

    template
    uint8_t convertUInt(uint8_t& val);
    template
    uint16_t convertUInt(uint8_t& val);
    template
    uint32_t convertUInt(uint8_t& val);
    template
    uint64_t convertUInt(uint8_t& val);
    template
    uint8_t convertUInt(uint16_t& val);
    template
    uint16_t convertUInt(uint16_t& val);
    template
    uint32_t convertUInt(uint16_t& val);
    template
    uint64_t convertUInt(uint16_t& val);
    template
    uint8_t convertUInt(uint32_t& val);
    template
    uint16_t convertUInt(uint32_t& val);
    template
    uint32_t convertUInt(uint32_t& val);
    template
    uint64_t convertUInt(uint32_t& val);
    template
    uint8_t convertUInt(uint64_t& val);
    template
    uint16_t convertUInt(uint64_t& val);
    template
    uint32_t convertUInt(uint64_t& val);
    template
    uint64_t convertUInt(uint64_t& val);

    template
    int8_t convertInt(int8_t& val);
    template
    int16_t convertInt(int8_t& val);
    template
    int32_t convertInt(int8_t& val);
    template
    int64_t convertInt(int8_t& val);
    template
    int8_t convertInt(int16_t& val);
    template
    int16_t convertInt(int16_t& val);
    template
    int32_t convertInt(int16_t& val);
    template
    int64_t convertInt(int16_t& val);
    template
    int8_t convertInt(int32_t& val);
    template
    int16_t convertInt(int32_t& val);
    template
    int32_t convertInt(int32_t& val);
    template
    int64_t convertInt(int32_t& val);
    template
    int8_t convertInt(int64_t& val);
    template
    int16_t convertInt(int64_t& val);
    template
    int32_t convertInt(int64_t& val);
    template
    int64_t convertInt(int64_t& val);


    template
    int8_t uveMax(int8_t val1, int8_t val2);
    template
    int16_t uveMax(int16_t val1, int16_t val2);
    template
    int32_t uveMax(int32_t val1, int32_t val2);
    template
    int64_t uveMax(int64_t val1, int64_t val2);
    template
    uint8_t uveMax(uint8_t val1, uint8_t val2);
    template
    uint16_t uveMax(uint16_t val1, uint16_t val2);
    template
    uint32_t uveMax(uint32_t val1, uint32_t val2);
    template
    uint64_t uveMax(uint64_t val1, uint64_t val2);
    template
    float uveMax(float val1, float val2);
    template
    double uveMax(double val1, double val2);

    template
    int8_t uveMin(int8_t val1, int8_t val2);
    template
    int16_t uveMin(int16_t val1, int16_t val2);
    template
    int32_t uveMin(int32_t val1, int32_t val2);
    template
    int64_t uveMin(int64_t val1, int64_t val2);
    template
    uint8_t uveMin(uint8_t val1, uint8_t val2);
    template
    uint16_t uveMin(uint16_t val1, uint16_t val2);
    template
    uint32_t uveMin(uint32_t val1, uint32_t val2);
    template
    uint64_t uveMin(uint64_t val1, uint64_t val2);
    template
    float uveMin(float val1, float val2);
    template
    double uveMin(double val1, double val2);


    template
    bool uveEGT(int8_t val1, int8_t val2);
    template
    bool uveEGT(int16_t val1, int16_t val2);
    template
    bool uveEGT(int32_t val1, int32_t val2);
    template
    bool uveEGT(int64_t val1, int64_t val2);
    template
    bool uveEGT(uint8_t val1, uint8_t val2);
    template
    bool uveEGT(uint16_t val1, uint16_t val2);
    template
    bool uveEGT(uint32_t val1, uint32_t val2);
    template
    bool uveEGT(uint64_t val1, uint64_t val2);
    template
    bool uveEGT(float val1, float val2);
    template
    bool uveEGT(double val1, double val2);

    template
    bool uveEQ(int8_t val1, int8_t val2);
    template
    bool uveEQ(int16_t val1, int16_t val2);
    template
    bool uveEQ(int32_t val1, int32_t val2);
    template
    bool uveEQ(int64_t val1, int64_t val2);
    template
    bool uveEQ(uint8_t val1, uint8_t val2);
    template
    bool uveEQ(uint16_t val1, uint16_t val2);
    template
    bool uveEQ(uint32_t val1, uint32_t val2);
    template
    bool uveEQ(uint64_t val1, uint64_t val2);
    template
    bool uveEQ(float val1, float val2);
    template
    bool uveEQ(double val1, double val2);

    template
    bool uveLT(int8_t val1, int8_t val2);
    template
    bool uveLT(int16_t val1, int16_t val2);
    template
    bool uveLT(int32_t val1, int32_t val2);
    template
    bool uveLT(int64_t val1, int64_t val2);
    template
    bool uveLT(uint8_t val1, uint8_t val2);
    template
    bool uveLT(uint16_t val1, uint16_t val2);
    template
    bool uveLT(uint32_t val1, uint32_t val2);
    template
    bool uveLT(uint64_t val1, uint64_t val2);
    template
    bool uveLT(float val1, float val2);
    template
    bool uveLT(double val1, double val2);

    template
    uint8_t castBitsToRetType(uint64_t arg);
    template
    uint16_t castBitsToRetType(uint64_t arg);
    template
    uint32_t castBitsToRetType(uint64_t arg);
    template
    uint64_t castBitsToRetType(uint64_t arg);
    template
    int8_t castBitsToRetType(uint64_t arg);
    template
    int16_t castBitsToRetType(uint64_t arg);
    template
    int32_t castBitsToRetType(uint64_t arg);
    template
    int64_t castBitsToRetType(uint64_t arg);
    template
    float castBitsToRetType(uint64_t arg);
    template
    double castBitsToRetType(uint64_t arg);

    template
    uint64_t typeCastToBits(uint8_t arg);
    template
    uint64_t typeCastToBits(uint16_t arg);
    template
    uint64_t typeCastToBits(uint32_t arg);
    template
    uint64_t typeCastToBits(uint64_t arg);
    template
    uint64_t typeCastToBits(int8_t arg);
    template
    uint64_t typeCastToBits(int16_t arg);
    template
    uint64_t typeCastToBits(int32_t arg);
    template
    uint64_t typeCastToBits(int64_t arg);
    template
    uint64_t typeCastToBits(float arg);
    template
    uint64_t typeCastToBits(double arg);
} // namespace RiscvISA
