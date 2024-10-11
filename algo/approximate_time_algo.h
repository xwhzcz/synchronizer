/**
 * @file approximate_time_algo.h
 * @author Wenhao Xin (wenhao.xin@liangdao.ai)
 * @brief
 * @version 0.1
 * @date 2023-08-11
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <list>

#include "algo/base_sync_algo.h"

class ApproximateTimeAlgo : public BaseSyncAlog {
 public:
  explicit ApproximateTimeAlgo(ChannelContainer* channels)
      : channels_(channels) {}
  ~ApproximateTimeAlgo() override = default;
  void sync(const DataCb& data_cb) override;
  void set_mini_time_interval(TimestampType interval) override;

 private:
  void wait_data() const;
  void init();
  void select_pivot();
  void select_data();
  void create_order(const std::string& name);
  void update_order();
  void update_mini_set();
  TimestampType cur_time(const std::string& name);
  TimestampType cur_size();
  BaseMsgHolder::SharedPtr cur_point(const std::string& name);
  void move();
  BaseMsgHolder::SharedPtr mini_point(const std::string& name);
  void clear_channel_data() const;

  OutDataType out_data_;
  struct {
    std::string pivot_;
    std::unordered_map<std::string, int> cur_pos_;
    std::unordered_map<std::string, TimestampType> cur_time_;
    std::unordered_map<std::string, int> mini_pos_;
    std::list<std::string> cur_order_;
    TimestampType mini_size_;
  } status_;
  ChannelContainer* channels_;
  std::mutex mtx_;
  TimestampType mini_interval_{0};
};
