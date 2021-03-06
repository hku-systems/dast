#include "piece.h"
#include <limits>

namespace rococo {

using rococo::RccDTxn;

char TPCC_TB_WAREHOUSE[] =    "warehouse";
char TPCC_TB_DISTRICT[] =     "district";
char TPCC_TB_CUSTOMER[] =     "customer";
char TPCC_TB_HISTORY[] =      "history";
char TPCC_TB_ORDER[] =        "order";
char TPCC_TB_NEW_ORDER[] =    "new_order";
char TPCC_TB_ITEM[] =         "item";
char TPCC_TB_STOCK[] =        "stock";
char TPCC_TB_ORDER_LINE[] =   "order_line";
char TPCC_TB_ORDER_C_ID_SECONDARY[] = "order_secondary";
char TPCC_TB_REGION[] = "region";
char TPCC_TB_NATION[] = "nation";
char TPCC_TB_SUPPLIER[] = "supplier";

void TpccPiece::reg_all() {
    RegNewOrder();
    RegPayment();
    RegOrderStatus();
    RegDelivery();
    RegStockLevel();
    RegQuery2();
}

std::set<std::pair<innid_t, innid_t>> TpccPiece::conflicts_ {};



TpccPiece::TpccPiece() {
  Log_info("%s called", __FUNCTION__ );
   for(int i = 0; i < 1000; i++){
     conflicts_.insert(std::make_pair(18000+i,1000));
     conflicts_.insert(std::make_pair(18000+i,303));
     conflicts_.insert(std::make_pair(18000+i,402));
     conflicts_.insert(std::make_pair(18000+i,501));
     for(int j = 0; j < 1000; j++){
       conflicts_.insert(std::make_pair(18000+i,1000+j));
       conflicts_.insert(std::make_pair(52000+i,17000+j));
       conflicts_.insert(std::make_pair(61000+i,63000+j));
       conflicts_.insert(std::make_pair(62000+i,17000+j));
       conflicts_.insert(std::make_pair(63000+i,61000+j));
       conflicts_.insert(std::make_pair(63000+i,63000+j));
     }
     conflicts_.insert(std::make_pair(303,18000+i));
     conflicts_.insert(std::make_pair(402,18000+i));
     conflicts_.insert(std::make_pair(501,18000+i));
   }
   conflicts_.insert(std::make_pair(1004,1000));
   conflicts_.insert(std::make_pair(1004,1004));
   conflicts_.insert(std::make_pair(1004,400));
   conflicts_.insert(std::make_pair(204,1002));
   conflicts_.insert(std::make_pair(204,203));
   conflicts_.insert(std::make_pair(204,204));
   conflicts_.insert(std::make_pair(204,301));
   conflicts_.insert(std::make_pair(204,302));
   conflicts_.insert(std::make_pair(204,403));
   conflicts_.insert(std::make_pair(205,205));
   conflicts_.insert(std::make_pair(301,204));
   conflicts_.insert(std::make_pair(301,403));
   conflicts_.insert(std::make_pair(302,1003));
   conflicts_.insert(std::make_pair(302,401));
   conflicts_.insert(std::make_pair(303,402));
   conflicts_.insert(std::make_pair(401,1003));
   conflicts_.insert(std::make_pair(401,302));
   conflicts_.insert(std::make_pair(401,401));
   conflicts_.insert(std::make_pair(402,303));
   conflicts_.insert(std::make_pair(402,402));
   conflicts_.insert(std::make_pair(402,501));
   conflicts_.insert(std::make_pair(403,1002));
   conflicts_.insert(std::make_pair(403,203));
   conflicts_.insert(std::make_pair(403,204));
   conflicts_.insert(std::make_pair(403,301));
   conflicts_.insert(std::make_pair(403,302));
   conflicts_.insert(std::make_pair(403,403));
   conflicts_.insert(std::make_pair(501,402));


}

TpccPiece::~TpccPiece() {}

}
