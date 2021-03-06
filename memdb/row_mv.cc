//
// Created by micha on 2020/3/27.
//

#include "row_mv.h"
namespace rococo {

void ChronosRow::init_ver(int n_columns) {
    std::unique_lock<std::recursive_mutex> lk(ver_lock);
  wver_.resize(n_columns, 0);
  rver_.resize(n_columns, 0);
  prepared_rver_.resize(n_columns, {});
  prepared_wver_.resize(n_columns, {});
}

void ChronosRow::copy_into(ChronosRow *row)  {
    std::unique_lock<std::recursive_mutex> lk(ver_lock);
  this->mdb::CoarseLockedRow::copy_into((mdb::CoarseLockedRow *) row);
  int n_columns = schema_->columns_count();
  row->init_ver(n_columns);
  row->wver_ = this->wver_;
  row->rver_ = this->rver_;
  row->prepared_rver_ = this->prepared_rver_;
  row->prepared_wver_ = this->prepared_wver_;

  verify(row->rver_.size() > 0);
}

mdb::Value ChronosRow::get_column_by_ver(int column_id, mdb::i64 txn_id, mdb::i64 ver) {
    std::unique_lock<std::recursive_mutex> lk(ver_lock);
  mdb::i64 current_ver = this->getCurrentVersion(column_id);
  mdb::Value v;
  if (ver >= current_ver){
    //already the newest
    v = Row::get_column(column_id);
    v.ver_ = current_ver;
  }
  else{
    //Getting an older version;
    auto itr = old_values_[column_id].rbegin();

    while(itr->first > ver){
      itr++; //reverse iterator ++
    }
    //itr->first is the max version smaller than ver
    verify(itr != old_values_[column_id].rend());

    v = itr->second;
    v.ver_ = itr->first;
  }
  return v;
}



//void ChronosRow::update_with_ver(int column_id, const Value& v, i64 ver, i64 txn_id) {
//  switch (v.get_kind()) {
//    case Value::I32:
//      this->update_with_ver(column_id, v.get_i32(), ver, txn_id);
//      break;
//    case Value::I64:
//      this->update_with_ver(column_id, v.get_i64(), ver, txn_id);
//      break;
//    case Value::DOUBLE:
//      this->update_with_ver(column_id, v.get_double(), ver, txn_id);
//      break;
//    case Value::STR:
//      this->update_with_ver(column_id, v.get_str(), ver, txn_id);
//      break;
//    default:
//      Log::fatal("unexpected value type %d", v.get_kind());
//      verify(0);
//      break;
//
//  }
//}


}//namespace
