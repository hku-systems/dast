//
// Created by micha on 2020/3/27.
//

#ifndef ROCOCO_MEMDB_ROW_MV_H_
#define ROCOCO_MEMDB_ROW_MV_H_

#include "memdb/row.h"
#include <mutex>

namespace rococo {

class ChronosRow : public mdb::CoarseLockedRow {
  using version_t = mdb::version_t;
  using column_id_t  = mdb::column_id_t;
  using Value = mdb::Value;
  using i64 = mdb::i64;
public:
  std::vector<std::list<version_t>> prepared_rver_{};
  std::vector<std::list<version_t>> prepared_wver_{};

  std::vector<version_t> wver_{};
  std::vector<version_t> rver_{};

  std::recursive_mutex ver_lock;

public:
  ~ChronosRow() {
      //no need
  }

  void init_ver(int n_columns);

  void copy_into(ChronosRow *row);

  virtual mdb::symbol_t rtti() const {
      return mdb::symbol_t::ROW_CHRONOS;
  }

  virtual Row *copy() {
      ChronosRow *row = new ChronosRow();
      copy_into(row);
      return row;
  }

  version_t max_prepared_rver(column_id_t column_id) {
      std::unique_lock<std::recursive_mutex> vl(ver_lock);
      if (prepared_wver_[column_id].size() > 0) {
          return prepared_wver_[column_id].back();
      } else {
          return 0;
      }
  }

  version_t min_prepared_rver(column_id_t column_id) {
      std::unique_lock<std::recursive_mutex> lk(ver_lock);
      if (prepared_wver_[column_id].size() > 0) {
          return prepared_wver_[column_id].front();
      } else {
          return 0;
      }
  }

  version_t min_prepared_wver(column_id_t column_id) {
      std::unique_lock<std::recursive_mutex> lk(ver_lock);
      if (prepared_rver_[column_id].size() > 0) {
          return prepared_rver_[column_id].front();
      } else {
          return 0;
      }
  }

  version_t max_prepared_wver(column_id_t column_id) {
      std::unique_lock<std::recursive_mutex> lk(ver_lock);
      if (prepared_rver_[column_id].size() > 0) {
          return prepared_rver_[column_id].back();
      } else {
          return 0;
      }
  }

  void insert_prepared_wver(column_id_t column_id, version_t ver) {
      std::unique_lock<std::recursive_mutex> lk(ver_lock);
      prepared_wver_[column_id].push_back(ver);
      prepared_rver_[column_id].sort(); // TODO optimize
  }

  void remove_prepared_wver(column_id_t column_id, version_t ver) {
      std::unique_lock<std::recursive_mutex> lk(ver_lock);
      prepared_wver_[column_id].remove(ver);
  }

  void insert_prepared_rver(column_id_t column_id, version_t ver) {
      std::unique_lock<std::recursive_mutex> lk(ver_lock);
      prepared_rver_[column_id].push_back(ver);
      prepared_rver_[column_id].sort(); // TODO optimize
  }

  void remove_prepared_rver(column_id_t column_id, version_t ver) {
      std::unique_lock<std::recursive_mutex> lk(ver_lock);
      prepared_rver_[column_id].remove(ver);
  }

  version_t get_column_rver(column_id_t column_id) {
      std::unique_lock<std::recursive_mutex> lk(ver_lock);
      verify(rver_.size() > 0);
      verify(column_id < rver_.size());
      return rver_[column_id];
  }

  version_t get_column_wver(column_id_t column_id) {
      std::unique_lock<std::recursive_mutex> lk(ver_lock);
      verify(wver_.size() > 0);
      verify(column_id < wver_.size());
      return wver_[column_id];
  }

  void set_column_rver(column_id_t column_id, version_t ver) {
      std::unique_lock<std::recursive_mutex> lk(ver_lock);
      rver_[column_id] = ver;
  }

  void set_column_wver(column_id_t column_id, version_t ver) {
      std::unique_lock<std::recursive_mutex> lk(ver_lock);
      wver_[column_id] = ver;
  }

  // data structure to keep all old versions for a row
  std::map<mdb::column_id_t, std::map<i64, Value> > old_values_ = {};
  // data structure to keep real time for each 100 versions. used for GC
  std::map<mdb::column_id_t, std::map<i64, std::map<i64, Value>::iterator> > time_segment;

  //xsTOOD: enable garbage colliection
//  void garbageCollection(int column_id, std::map<i64, Value>::iterator itr);


  //This is the entry function
  //no need to lock as the called function will call mutex
  void update_with_ver(int column_id, const Value &v, i64 txn_id, i64 ver) {
      update_internal(column_id, v, ver, txn_id);
  }
//
//  void update_with_ver(int column_id, i64 v, i64 txn_id, i64 ver) {
//    update_internal(column_id, v, ver, txn_id);
//  }
//
//  void update_with_ver(int column_id, i32 v, i64 txn_id, i64 ver) {
//    update_internal(column_id, v, ver, txn_id);
//  }
//
//  void update_with_ver(int column_id, double v, i64 txn_id, i64 ver) {
//    update_internal(column_id, v, ver, txn_id);
//  }
//
//  void update_with_ver(int column_id, const std::string &str, i64 txn_id, i64 ver) {
//    update_internal(column_id, str, ver, txn_id);
//  }
//

  /*
   * For reads, we need the id for this read txn
   */


  // retrieve current version number
  version_t getCurrentVersion(int column_id) {
      std::unique_lock<std::recursive_mutex> lk(ver_lock);
      return wver_[column_id];
  }

  // get a value specified by a version number
  Value get_column_by_ver(int column_id, i64 txn_number, i64 version_num);

  template<class Container>
  static ChronosRow *create(const mdb::Schema *schema, const Container &values) {
      verify(values.size() == schema->columns_count());
      std::vector<const Value *> values_ptr(values.size(), nullptr);
      size_t fill_counter = 0;
      for (auto it = values.begin(); it != values.end(); ++it) {
          fill_values_ptr(schema, values_ptr, *it, fill_counter);
          fill_counter++;
      }
      ChronosRow *raw_row = new ChronosRow();
      raw_row->init_ver(schema->columns_count());
      return (ChronosRow *) Row::create(raw_row, schema, values_ptr);
      // TODO (for haonan) initialize the data structure for a new row here.
  }

  template<typename Type>
  void update_internal(int column_id, Type v, i64 ver, i64 txn_id) {
      std::unique_lock<std::recursive_mutex> lk(ver_lock);
      // first get current value before update, and put current value in old_values_
      Value currentValue = this->Row::get_column(column_id);
      // get current version
      version_t currentVersionNumber = getCurrentVersion(column_id);

      if (ver > currentVersionNumber){
          //step 1: insert the current version into old_values;
          std::pair<i64, Value> valueEntry = std::make_pair(currentVersionNumber, currentValue);
          old_values_[column_id].insert(valueEntry);

          //step 2: update current version
          Row::update(column_id, v);
          this->wver_[column_id] = ver;
      }
      else{
          std::pair<i64, Value> valueEntry = std::make_pair(ver, v);
          old_values_[column_id].insert(valueEntry);
      }


      // map.insert will not insert to the map (i.e., not overwrite) if there is already elementes with the same key in the map.
      // returns the element to the key (either the original or the newly inserted.
//    std::map<i64, Value>::iterator newElementItr = (old_values_[column_id].insert(valueEntry)).first;


//    if (old_values_[column_id].size() % GC_THRESHOLD == 0) {
//      // do Garbage Collection
//      garbageCollection(column_id, newElementItr);
//    }


      //Now we need to update rtxn_tracker first, seems not useful for chronos
//    for (i64 txnId : txnIds) {
//      rtxn_tracker.checkIfTxnIdBeenRecorded(column_id, txnId, true, currentVersionNumber);
//    }
      // then update column as normal

  }

};

}
#endif //ROCOCO_MEMDB_ROW_MV_H_
