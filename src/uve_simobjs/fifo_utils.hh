#ifndef __UVE_SIMOBJS_FIFO_UTILS_HH__
#define __UVE_SIMOBJS_FIFO_UTILS_HH__

#include "utils.hh"

template <class BaseContainer>
class DumbIterator {
   private:
    int it;
    BaseContainer *container;
    bool reverse;

   public:
    DumbIterator(BaseContainer *cnt, bool reverse_iter = true)
        : it(-1), container(cnt), reverse(reverse_iter){};
    DumbIterator(bool reverse_iter = true) : it(-1), reverse(reverse_iter){};
    ~DumbIterator(){};

    void setContainer(BaseContainer *cnt) { container = cnt; }

    typename BaseContainer::value_type operator*() {
        if (reverse) {
            auto aux = container->rbegin();
            advance(aux, it);
            return *aux;
        } else {
            auto aux = container->begin();
            advance(aux, it);
            return *aux;
        }
    }

    typename BaseContainer::value_type *operator->() {
        if (reverse) {
            auto aux = container->rbegin();
            advance(aux, it);
            return &(*aux);
        } else {
            auto aux = container->begin();
            advance(aux, it);
            return &(*aux);
        }
    }

    DumbIterator<BaseContainer> &operator++(int a) {
        it++;
        if (it > container->size()) it = container->size();
        return *this;
    }
    DumbIterator<BaseContainer> &operator--(int a) {
        it--;
        if (it < 0) it = -1;
        return *this;
    }
    bool __attribute_noinline__ valid() {
        return !(it == -1 || it == container->size());
    }
    void clear() { it = -1; }
    void forceUpdate() {
        if (it == -1 && container->size() > 0) it = 0;
    }
};

#endif //__UVE_SIMOBJS_FIFO_UTILS_HH__