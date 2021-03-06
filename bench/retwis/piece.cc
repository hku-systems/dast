#include "piece.h"
#include <limits>
#include "deptran/__dep__.h"
#include "../tpcc/piece.h"
namespace rococo {

using rococo::RccDTxn;

char RETWIS_TB[] =  "history";



void RetwisPiece::reg_all() {
    RegAddUsers();
    RegFollow();
    RegPostTweet();
    RegGetTimeline();
}

RetwisPiece::RetwisPiece() {}

RetwisPiece::~RetwisPiece() {}

}
