//
// Created by tyycxs on 2020/5/7.
//

#ifndef ROCOCO_DEPTRAN_CHRONOS_TYPES_H
#define ROCOCO_DEPTRAN_CHRONOS_TYPES_H
#include "rrr.hpp"
#include <cstdint>
namespace rococo {

class chr_ts_t {
public:
    chr_ts_t(int64_t ts, int64_t counter, int16_t site_id) : timestamp_(ts), stretch_counter_(counter), site_id_(site_id) {}
    chr_ts_t() : timestamp_(0), stretch_counter_(0), site_id_(0) {}
    int64_t timestamp_;
    int64_t stretch_counter_;
    int16_t site_id_;
//    std::string to_string() const {
//        return std::to_string(timestamp_) + ":" + std::to_string(site_id_) + ":" + std::to_string(stretch_counter_);
//    }
    std::string to_string() const {
        return std::to_string(timestamp_) + ":" + std::to_string(stretch_counter_) + ":" + std::to_string(site_id_);
    }
//    inline bool operator<(const chr_ts_t rhs) const {
//        if (this->timestamp_ < rhs.timestamp_) {
//            return true;
//        } else if (this->timestamp_ > rhs.timestamp_) {
//            return false;
//        } else {
//            if (this->site_id_ < rhs.site_id_) {
//                return true;
//            } else if (this->site_id_ > rhs.site_id_) {
//                return false;
//            } else {
//                return this->stretch_counter_ < rhs.stretch_counter_;
//            }
//        }
//    }
    inline bool operator<(const chr_ts_t rhs) const {
        if (this->timestamp_ < rhs.timestamp_) {
            return true;
        } else if (this->timestamp_ > rhs.timestamp_) {
            return false;
        } else {
            if (this->stretch_counter_ < rhs.stretch_counter_) {
                return true;
            } else if (this->stretch_counter_ > rhs.stretch_counter_) {
                return false;
            } else {
                return this->site_id_ < rhs.site_id_;
            }
        }
    }

    inline bool operator==(const chr_ts_t rhs) const {
        return (this->timestamp_ == rhs.timestamp_ && this->stretch_counter_ == rhs.stretch_counter_ && this->site_id_ == rhs.site_id_);
    }

    inline bool operator<=(const chr_ts_t rhs) const {
        return (this->operator<(rhs) || this->operator==(rhs));
    }

    inline bool operator>(const chr_ts_t rhs) const {
        return (!this->operator<(rhs) && !this->operator==(rhs));
    }

    Marshal &ToMarshal(Marshal &m) const {
        m << timestamp_;
        m << stretch_counter_;
        m << site_id_;
        return m;
    }
    Marshal &FromMarshal(Marshal &m) {
        m >> timestamp_;
        m >> stretch_counter_;
        m >> site_id_;
        return m;
    }
};

rrr::Marshal &operator<<(rrr::Marshal &m, const chr_ts_t &chr_ts);
rrr::Marshal &operator>>(rrr::Marshal &m, chr_ts_t &chr_ts);


}// namespace rococo
#endif//ROCOCO_DEPTRAN_CHRONOS_TYPES_H
