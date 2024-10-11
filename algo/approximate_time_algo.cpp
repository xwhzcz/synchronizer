/**
 * @file approximate_time_algo.cpp
 * @author Wenhao Xin (wenhao.xin@liangdao.ai)
 * @brief
 * @version 0.1
 * @date 2023-08-11
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "algo/approximate_time_algo.h"

void ApproximateTimeAlgo::sync(const DataCb& data_cb) {
  running_.store(true);
  while (running_.load()) {
    wait_data();
    if (!running_.load()) {
      break;
    }
    // 0. 清空缓存
    init();
    // 1. 选择数据
    select_data();
    if (!running_.load()) {
      break;
    }
    // 2. 发布数据
    data_cb(out_data_);
  }
}

void ApproximateTimeAlgo::set_mini_time_interval(TimestampType interval) {
  mini_interval_ = interval;
}

void ApproximateTimeAlgo::wait_data() const {
  for (const auto& [name, channel] : *channels_) {
#ifdef DEBUG
    std::cout << "Algo channel wait" << name << std::endl;
#endif
    if (!running_.load()) {
      break;
    }
    channel->wait_not_empty();
  }
}

void ApproximateTimeAlgo::init() {
  out_data_.clear();
  status_.cur_pos_.clear();
  status_.cur_time_.clear();
  status_.mini_pos_.clear();
  status_.cur_order_.clear();
  status_.mini_size_ = TIMESTAMP_MAX;
}

void ApproximateTimeAlgo::select_pivot() {
  for (const auto& [name, time] : status_.cur_time_) {
    if (status_.pivot_ == "") status_.pivot_ = name;
    if (cur_time(status_.pivot_) <= time) {
      status_.pivot_ = name;
    }
  }
}

void ApproximateTimeAlgo::select_data() {
  for (const auto& [name, channel] : *channels_) {
    status_.cur_pos_[name] = 0;
    status_.cur_time_[name] = cur_point(name)->time();
    create_order(name);
  }
  update_mini_set();

  select_pivot();
  while (status_.cur_order_.back() != status_.pivot_) {
    if (mini_interval_ != 0 && status_.mini_size_ < mini_interval_) break;
    move();
    if (!running_.load()) {
      return;
    }
  }
  for (const auto& [name, pos] : status_.mini_pos_) {
    auto channel = channels_->at(name);
    std::unique_lock lk(channel->mtx());
    out_data_[name] = channel->at(pos);
    channel->erase(0, pos + 1);
  }
}

void ApproximateTimeAlgo::update_mini_set() {
  auto size = cur_size();
  if (status_.mini_size_ >= size) {
    status_.mini_size_ = size;
    status_.mini_pos_ = status_.cur_pos_;
  }
}

TimestampType ApproximateTimeAlgo::cur_time(const std::string& name) {
  return status_.cur_time_[name];
}

TimestampType ApproximateTimeAlgo::cur_size() {
  return cur_time(status_.cur_order_.front()) -
         cur_time(status_.cur_order_.back());
}

BaseMsgHolder::SharedPtr ApproximateTimeAlgo::cur_point(
    const std::string& name) {
  return channels_->at(name)->concurrent_at(status_.cur_pos_[name]);
}

void ApproximateTimeAlgo::move() {
  auto moved_point_name = status_.cur_order_.back();
  auto next_point_pos = status_.cur_pos_[moved_point_name] + 1;
  auto msg = channels_->at(moved_point_name)->wait_at(next_point_pos);
  if (!msg) {
    return;
  }
  status_.cur_time_[moved_point_name] = msg->time();
  status_.cur_pos_[moved_point_name] = next_point_pos;
  update_order();
  update_mini_set();
}

BaseMsgHolder::SharedPtr ApproximateTimeAlgo::mini_point(
    const std::string& name) {
  return channels_->at(name)->concurrent_at(status_.mini_pos_[name]);
}

void ApproximateTimeAlgo::clear_channel_data() const {
  for (const auto& [name, pos] : status_.mini_pos_) {
    channels_->at(name)->concurrent_erase(0, pos + 1);
  }
}

void ApproximateTimeAlgo::update_order() {
  auto last = std::next(status_.cur_order_.end(), -1);
  const auto& name = *last;
  auto it = status_.cur_order_.begin();
  while (it != status_.cur_order_.end() && cur_time(name) < cur_time(*it)) ++it;
  status_.cur_order_.splice(it, status_.cur_order_, last);
}

void ApproximateTimeAlgo::create_order(const std::string& name) {
  auto it = status_.cur_order_.begin();
  while (it != status_.cur_order_.end() && cur_time(name) < cur_time(*it)) ++it;
  status_.cur_order_.emplace(it, name);
}
