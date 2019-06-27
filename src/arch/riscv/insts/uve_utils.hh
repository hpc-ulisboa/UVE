
#ifndef __ARCH_RISCV_UVE_UTILS_HH__
#define __ARCH_RISCV_UVE_UTILS_HH__

#include <stdint.h>

#include "arch/riscv/insts/static_inst.hh"
#include "debug/UVEUtils.hh"

namespace RiscvISA
{
    #define MEM_PRINTF(mem,dir,vec,count)  DPRINTF(UVEMem, "______________" \
                "__________________\n"); \
                std::string ss; \
                for (int i = 0; i < count; i++) { \
                    ss += "(" + std::to_string(i) + ") M[" + \
                    to_hex(mem[i]) + "]" dir "V[" + \
                    std::to_string(vec[i]) + "] -"; \
                    if (i % 30 == 29) ss += "\\n"; \
                } \
                DPRINTF(UVEMem, (ss+="\n").c_str() ); \
                DPRINTF(UVEMem, "________________________________\n");


    template <typename T>
    std::string to_hex(T a){
        std::string result;
        std::stringstream ss;
        ss << std::hex << (long int) a;
        ss >> result;
        return result;
    }

    template <typename Ret>
    Ret convertFP(float& val);
    template <typename Ret>
    Ret convertFP(double& val);

    template <typename Ret>
    Ret convertUInt(uint8_t& val);
    template <typename Ret>
    Ret convertUInt(uint16_t& val);
    template <typename Ret>
    Ret convertUInt(uint32_t& val);
    template <typename Ret>
    Ret convertUInt(uint64_t& val);

    template <typename Ret>
    Ret convertInt(int8_t& val);
    template <typename Ret>
    Ret convertInt(int16_t& val);
    template <typename Ret>
    Ret convertInt(int32_t& val);
    template <typename Ret>
    Ret convertInt(int64_t& val);

    void check_equal_src_widths(size_t a, size_t b);

    uint16_t get_dest_valid_index(uint16_t a, uint16_t b);
    uint16_t get_dest_valid_index(uint16_t a, uint16_t b, uint16_t c);

    size_t get_vector_width(ExecContext *xc, uint8_t reg );

    template <typename Ret>
    typename std::enable_if<std::is_integral<Ret>::value, Ret>::type
    uveAbs(Ret val){
        return abs(val);
    }

    template <typename Ret>
    typename std::enable_if<std::is_floating_point<Ret>::value, Ret>::type
    uveAbs(Ret val){
        return fabs(val);
    }

    template <typename Ret>
    typename std::enable_if<std::is_integral<Ret>::value, Ret>::type
    uveInc(Ret val){
        return val + 1;
    }

    template <typename Ret>
    typename std::enable_if<std::is_floating_point<Ret>::value, Ret>::type
    uveInc(Ret val){
        return val + 1.0f;
    }

    template <typename Ret>
    typename std::enable_if<std::is_integral<Ret>::value, Ret>::type
    uveDec(Ret val){
        return val - 1;
    }

    template <typename Ret>
    typename std::enable_if<std::is_floating_point<Ret>::value, Ret>::type
    uveDec(Ret val){
        return val - 1.0f;
    }

    template <typename Ret>
    Ret uveMax(Ret val1, Ret val2);

    template <typename Ret>
    Ret uveMin(Ret val1, Ret val2);

    template <typename Ret>
    Ret castBitsToRetType(uint64_t arg);

    template <typename T>
    uint64_t typeCastToBits(T arg);


    // JMNOTE: Template Specialization
    template <class WidthSize>
    struct width_converter{
        static const uint8_t width = 3;
    };
    template <>
    struct width_converter<uint32_t>{
        static const uint8_t width = 2;
    };
    template <>
    struct width_converter<uint16_t>{
        static const uint8_t width = 1;
    };
    template <>
    struct width_converter<uint8_t>{
        static const uint8_t width = 0;
    };
    template <>
    struct width_converter<int32_t>{
        static const uint8_t width = 2;
    };
    template <>
    struct width_converter<int16_t>{
        static const uint8_t width = 1;
    };
    template <>
    struct width_converter<int8_t>{
        static const uint8_t width = 0;
    };
    template <>
    struct width_converter<float>{
        static const uint8_t width = 2;
    };


} // namespace RiscvISA


#endif  // __ARCH_RISCV_UVE_UTILS_HH__
