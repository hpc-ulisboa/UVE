#ifndef __UVE_SE_REDU_UTILS_HH__
#define __UVE_SE_REDU_UTILS_HH__

typedef enum  {
    first = 0,
    second,
    third,
    forth,
    fifth,
    sixth,
    seventh,
    last,
    dh_size
}DimensionHop;

// class DimensionHopObject {
//     bool * container;

//    public:
//     DimensionHopObject() {
//         container = (bool *) malloc(sizeof(bool)*DimensionHop::dh_size);
//     }
//     ~DimensionHopObject() {free(container);}

//     DimensionHopObject * operator||(const DimensionHopObject * other) const {
//         DimensionHopObject * result = new DimensionHopObject();
//         for(DimensionHop i=0; i<DimensionHop::dh_size; i++){
//             result->set(i, container[i] || other->get(i));
//         }
//         return result;
//     }

//     DimensionHopObject * operator&&(const DimensionHopObject * other) const {
//         DimensionHopObject * result = new DimensionHopObject();
//         for(DimensionHop i=0; i<DimensionHop::dh_size; i++){
//             result->set(i, container[i] && other->get(i));
//         }
//         return result;
//     }

//     bool get(DimensionHop hop){ return container[hop]; }
//     bool get(const DimensionHop hop) const { return container[hop]; }
//     void set(DimensionHop hop, bool val){ container[hop] = val; }
//     void set(DimensionHop hop){ container[hop] = true; }
//     void reset(DimensionHop hop){ container[hop] = false; }

//     std::string print(){
//         std::stringstream ostr;
//         ostr << "[";
//         for(int i=0; i<DimensionHop::dh_size; i++){
//             if(container[i] && i == DimensionHop::last){
//                 ostr << "L";
//                 break; 
//             }
//             if(container[i]) ostr << (int) i << ":";
//         }
//         ostr << "]";
//         return ostr.str();
//     }
// };

#endif //__UVE_SE_REDU_UTILS_HH__
