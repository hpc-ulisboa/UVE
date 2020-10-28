/*
 * Author: João Domingos
 */

#ifndef __UVE_VEC_REG_HH__
#define __UVE_VEC_REG_HH__

#include <array>
#include <cassert>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "base/cprintf.hh"
#include "base/logging.hh"
#include "debug/UVECONTAINER.hh"

#include "uve_simobjs/reduced_utils.hh"

// JMNOTE: This number must aggree to types.hh MaxUveVeccLenInBits
constexpr unsigned MaxVecRegLenInBytes = 256;
typedef int64_t vecSubStreamID;

template <size_t Sz>
class VecRegContainer;

template <typename VecElem, size_t NumElems, bool Const>
class VecRegT {
    static constexpr size_t SIZE = sizeof(VecElem) * NumElems;

   public:
    using Container =
        typename std::conditional<Const, const VecRegContainer<SIZE>,
                                  VecRegContainer<SIZE>>::type;

   private:
    using MyClass = VecRegT<VecElem, NumElems, Const>;
    Container& container;

   public:
    VecRegT(Container& cnt) : container(cnt){};

    template <bool Condition = !Const>
    typename std::enable_if<Condition, void>::type zero() {
        container.zero();
    }

    template <bool Condition = !Const>
    typename std::enable_if<Condition, MyClass&>::type operator=(
        const MyClass& that) {
        container = that.container;
        return *this;
    }

    const VecElem& operator[](size_t idx) const {
        return container.template raw_ptr<VecElem>()[idx];
    }

    template <bool Condition = !Const>
    typename std::enable_if<Condition, VecElem&>::type operator[](size_t idx) {
        return container.template raw_ptr<VecElem>()[idx];
    }

    template <typename VE2, size_t NE2, bool C2>
    bool operator==(const VecRegT<VE2, NE2, C2>& that) const {
        return container == that.container;
    }
    template <typename VE2, size_t NE2, bool C2>
    bool operator!=(const VecRegT<VE2, NE2, C2>& that) const {
        return !operator==(that);
    }

    void operator<=(const uint8_t width) const {
        return container.set_width(width);
    }

    void operator<<=(const uint16_t valid_index) const {
        return container.set_valid(valid_index);
    }

    void operator&=(const uint64_t last) const {
        return container.set_last(last);
    }

    friend std::ostream& operator<<(std::ostream& os, const MyClass& vr) {
        if (sizeof(VecElem) == 1) {
            char* val = (char*)malloc(sizeof(char) * 20);
            sprintf(val, "%x", (uint8_t)vr[0]);
            os << "["
               << "0x" << std::hex << val;
            for (uint64_t e = 1; e < NumElems; e++) {
                sprintf(val, "%x", (uint8_t)vr[e]);
                os << " 0x" << std::hex << val;
            }
            os << ']';
            return os;
        } else {
            os << "["
               << "0x" << std::hex << (VecElem)vr[0];
            for (uint64_t e = 1; e < NumElems; e++)
                os << " 0x" << std::hex << (VecElem)vr[e];
            os << ']';
            return os;
        }
    }

    const std::string print() const { return csprintf("%s", *this); }
    operator Container&() { return container; }

    void set_width(uint8_t _width) { container.set_width(); }
    void set_valid(uint16_t _valid) { container.set_valid(); }
    uint8_t get_width() const { return container.get_width(); }
    uint8_t get_valid() const { return container.get_valid(); }
    bool is_last(DimensionHop hop) const { return container.is_last(hop); }
    bool is_last(uint64_t hop) const { return container.is_last(hop); }
    bool get_last() const { return container.get_last(); }
};

template <typename VecElem, bool Const>
class VecLaneT;

template <size_t Sz>
class VecRegContainer {
    static_assert(Sz > 0,
                  "Cannot create Vector Register Container of zero size");
    static_assert(Sz <= MaxVecRegLenInBytes,
                  "Vector Register size limit exceeded");

   public:
    static constexpr size_t SIZE = Sz;
    using Container = std::array<uint8_t, Sz>;

   private:
    Container container;
    using MyClass = VecRegContainer<SIZE>;
    uint8_t width;
    uint16_t valid_index;
    uint64_t dhobject;
    // DEBUG only flags/vars
    bool streaming;
    vecSubStreamID ssid;
    int sid;

   public:
    VecRegContainer()
        : width(0),
          valid_index(0),
          dhobject(0),
          streaming(false),
          ssid(-1),
          sid(-1) {}
    VecRegContainer(const std::vector<uint8_t>& that) {
        assert(that.size() >= SIZE);
        std::memcpy(container.data(), &that[0], SIZE);
    }

    void set_width(uint8_t _width) { width = _width; }
    void set_valid(uint16_t _valid) { valid_index = _valid; }
    uint8_t get_width() const { return width; }
    uint8_t get_valid() const { return valid_index; }
    void set_last(uint64_t hobj) { 
        dhobject = hobj;
    }
    void set_last(DimensionHop hop, bool val = true) { 
        if(val)
            dhobject |= 1 << hop;
        else
            dhobject &= 0 << hop;
    }
    void reset_last(DimensionHop hop) { 
        set_last(hop, false);
    }
    bool is_last(DimensionHop hop) const {
            return (dhobject & (1 << hop)) > 0 ? true : false;
        }
    bool is_last(uint64_t hop) const {
            return (dhobject & (1 << hop)) > 0 ? true : false;
        }
    uint64_t get_last() const { return dhobject; }
    void set_streaming(bool _streaming) { streaming = _streaming; }
    bool is_streaming() const { return streaming; }
    void set_sid(int _sid) { sid = _sid; }
    void set_ssid(vecSubStreamID _ssid) { ssid = _ssid; }
    vecSubStreamID get_ssid() const { return ssid; }
    int get_sid() { return sid; }

    void zero() { memset(container.data(), 0, SIZE); }

    MyClass& operator=(const MyClass& that) {
        if (&that == this) return *this;
        memcpy(container.data(), that.container.data(), SIZE);
        width = that.width;
        valid_index = that.valid_index;
        dhobject = that.dhobject;
        streaming = that.streaming;
        sid = that.sid;
        ssid = that.ssid;
        return *this;
    }

    MyClass& operator=(const Container& that) {
        std::memcpy(container.data(), that.data(), SIZE);
        return *this;
    }

    MyClass& operator=(const std::vector<uint8_t>& that) {
        assert(that.size() >= SIZE);
        std::memcpy(container.data(), that.data(), SIZE);
        return *this;
    }

    void copyTo(Container& dst) const {
        std::memcpy(dst.data(), container.data(), SIZE);
    }

    void copyTo(std::vector<uint8_t>& dst) const {
        dst.resize(SIZE);
        std::memcpy(dst.data(), container.data(), SIZE);
    }

    template <size_t S2>
    inline bool operator==(const VecRegContainer<S2>& that) const {
        return SIZE == S2 &&
               !memcmp(container.data(), that.container.data(), SIZE);
    }

    template <size_t S2>
    bool operator!=(const VecRegContainer<S2>& that) const {
        return !operator==(that);
    }

    const std::string print() const { return csprintf("%s", *this); }

    template <typename Ret>
    const Ret* raw_ptr() const {
        return (const Ret*)container.data();
    }

    template <typename Ret>
    Ret* raw_ptr() {
        return (Ret*)container.data();
    }

    template <typename VecElem, size_t NumElems = SIZE / sizeof(VecElem)>
    VecRegT<VecElem, NumElems, true> as() const {
        static_assert(SIZE % sizeof(VecElem) == 0,
                      "VecElem does not evenly divide the register size");
        static_assert(sizeof(VecElem) * NumElems <= SIZE,
                      "Viewing VecReg as something bigger than it is");
        return VecRegT<VecElem, NumElems, true>(*this);
    }

    template <typename VecElem, size_t NumElems = SIZE / sizeof(VecElem)>
    VecRegT<VecElem, NumElems, false> as() {
        static_assert(SIZE % sizeof(VecElem) == 0,
                      "VecElem does not evenly divide the register size");
        static_assert(sizeof(VecElem) * NumElems <= SIZE,
                      "Viewing VecReg as something bigger than it is");
        return VecRegT<VecElem, NumElems, false>(*this);
    }

    template <typename VecElem, int LaneIdx>
    VecLaneT<VecElem, false> laneView();
    template <typename VecElem, int LaneIdx>
    VecLaneT<VecElem, true> laneView() const;
    template <typename VecElem>
    VecLaneT<VecElem, false> laneView(int laneIdx);
    template <typename VecElem>
    VecLaneT<VecElem, true> laneView(int laneIdx) const;

    friend std::ostream& operator<<(std::ostream& os, const MyClass& v) {
        os << csprintf("W:%d, Valid:%d \t", v.width, v.valid_index);
        for (auto& b : v.container) {
            os << csprintf("%02x", b);
        }
        return os;
    }
};

enum class LaneSize {
    Empty = 0,
    Byte,
    TwoByte,
    FourByte,
    EightByte,
};

template <LaneSize LS>
class LaneData {
   public:
    using UnderlyingType = typename std::conditional<
        LS == LaneSize::EightByte, uint64_t,
        typename std::conditional<
            LS == LaneSize::FourByte, uint32_t,
            typename std::conditional<
                LS == LaneSize::TwoByte, uint16_t,
                typename std::conditional<LS == LaneSize::Byte, uint8_t,
                                          void>::type>::type>::type>::type;

   private:
    static constexpr auto ByteSz = sizeof(UnderlyingType);
    UnderlyingType _val;
    using MyClass = LaneData<LS>;

   public:
    template <typename T>
    explicit LaneData(
        typename std::enable_if<sizeof(T) == ByteSz, const T&>::type t)
        : _val(t) {}

    template <typename T>
    typename std::enable_if<sizeof(T) == ByteSz, MyClass&>::type operator=(
        const T& that) {
        _val = that;
        return *this;
    }
    template <typename T,
              typename std::enable_if<sizeof(T) == ByteSz, int>::type I = 0>
    operator T() const {
        return *static_cast<const T*>(&_val);
    }
};

template <LaneSize LS>
inline std::ostream&
operator<<(std::ostream& os, const LaneData<LS>& d) {
    return os << static_cast<typename LaneData<LS>::UnderlyingType>(d);
}

template <typename VecElem, bool Const>
class VecLaneT {
   public:
    friend VecLaneT<VecElem, !Const>;

    friend class VecRegContainer<8>;
    friend class VecRegContainer<16>;
    friend class VecRegContainer<32>;
    friend class VecRegContainer<64>;
    friend class VecRegContainer<128>;
    friend class VecRegContainer<MaxVecRegLenInBytes>;

    using MyClass = VecLaneT<VecElem, Const>;

   private:
    using Cont =
        typename std::conditional<Const, const VecElem, VecElem>::type;
    static_assert(!std::is_const<VecElem>::value || Const,
                  "Asked for non-const lane of const type!");
    static_assert(std::is_integral<VecElem>::value,
                  "VecElem type is not integral!");
    Cont& container;

    VecLaneT(Cont& cont) : container(cont) {}

   public:
    template <bool Assignable = !Const>
    typename std::enable_if<Assignable, MyClass&>::type operator=(
        const VecElem& that) {
        container = that;
        return *this;
    }
    template <bool Assignable = !Const, typename T>
    typename std::enable_if<Assignable, MyClass&>::type operator=(
        const T& that) {
        static_assert(sizeof(T) >= sizeof(VecElem),
                      "Attempt to perform widening bitwise copy.");
        static_assert(sizeof(T) <= sizeof(VecElem),
                      "Attempt to perform narrowing bitwise copy.");
        container = static_cast<VecElem>(that);
        return *this;
    }
    operator VecElem() const { return container; }

    template <bool Cond = !Const, typename std::enable_if<Cond, int>::type = 0>
    operator VecLaneT<typename std::enable_if<Cond, VecElem>::type, true>() {
        return VecLaneT<VecElem, true>(container);
    }
};

namespace std {
    template <typename T, bool Const>
    struct add_const<VecLaneT<T, Const>> {
        typedef VecLaneT<T, true> type;
    };
}  // namespace std

template <size_t Sz>
template <typename VecElem, int LaneIdx>
VecLaneT<VecElem, false>
VecRegContainer<Sz>::laneView() {
    return VecLaneT<VecElem, false>(as<VecElem>()[LaneIdx]);
}

template <size_t Sz>
template <typename VecElem, int LaneIdx>
VecLaneT<VecElem, true>
VecRegContainer<Sz>::laneView() const {
    return VecLaneT<VecElem, true>(as<VecElem>()[LaneIdx]);
}

template <size_t Sz>
template <typename VecElem>
VecLaneT<VecElem, false>
VecRegContainer<Sz>::laneView(int laneIdx) {
    return VecLaneT<VecElem, false>(as<VecElem>()[laneIdx]);
}

template <size_t Sz>
template <typename VecElem>
VecLaneT<VecElem, true>
VecRegContainer<Sz>::laneView(int laneIdx) const {
    return VecLaneT<VecElem, true>(as<VecElem>()[laneIdx]);
}

using VecLane8 = VecLaneT<uint8_t, false>;
using VecLane16 = VecLaneT<uint16_t, false>;
using VecLane32 = VecLaneT<uint32_t, false>;
using VecLane64 = VecLaneT<uint64_t, false>;

using ConstVecLane8 = VecLaneT<uint8_t, true>;
using ConstVecLane16 = VecLaneT<uint16_t, true>;
using ConstVecLane32 = VecLaneT<uint32_t, true>;
using ConstVecLane64 = VecLaneT<uint64_t, true>;

template <size_t Sz>
inline bool
to_number(const std::string& value, VecRegContainer<Sz>& v) {
    fatal_if(value.size() > 2 * VecRegContainer<Sz>::SIZE,
             "Vector register value overflow at unserialize");

    for (int i = 0; i < VecRegContainer<Sz>::SIZE; i++) {
        uint8_t b = 0;
        if (2 * i < value.size())
            b = stoul(value.substr(i * 2, 2), nullptr, 16);
        v.template raw_ptr<uint8_t>()[i] = b;
    }
    return true;
}

using DummyVecElem = uint32_t;
constexpr unsigned DummyNumVecElemPerVecReg = 2;
using DummyVecReg = VecRegT<DummyVecElem, DummyNumVecElemPerVecReg, false>;
using DummyConstVecReg = VecRegT<DummyVecElem, DummyNumVecElemPerVecReg, true>;
using DummyVecRegContainer = DummyVecReg::Container;
constexpr size_t DummyVecRegSizeBytes =
    DummyNumVecElemPerVecReg * sizeof(DummyVecElem);

#endif /* __UVE_VEC_REG_HH__ */
