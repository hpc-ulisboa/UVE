
#include "cpu/o3/isa_specific.hh"
#include "cpu/o3/se_interface_impl.hh"

template <class Impl>
SEInterface<Impl>* SEInterface<Impl>::singleton = nullptr;
template class SEInterface<O3CPUImpl>;