//
// Created by tyycxs on 2020/5/7.
//

#ifndef ROCOCO_DEPTRAN_CHRONOS_TYPES_MARSHALL_CC
#define ROCOCO_DEPTRAN_CHRONOS_TYPES_MARSHALL_CC

#include "__dep__.h"
#include "deptran/chronos/types.h"

namespace rococo{

rrr::Marshal &operator<<(rrr::Marshal &m, const chr_ts_t &chr_ts){
  chr_ts.ToMarshal(m);
  return m;
}

rrr::Marshal &operator>>(rrr::Marshal &m, chr_ts_t &chr_ts){
  chr_ts.FromMarshal(m);
  return m;
}


};

#endif //ROCOCO_DEPTRAN_CHRONOS_TYPES_MARSHALL_CC
