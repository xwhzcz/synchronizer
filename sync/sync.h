/**
 * @file sync.h
 * @author Wenhao Xin (wenhao.xin@liangdao.ai)
 * @brief
 * @version 0.1
 * @date 2023-08-09
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <thread>

#include "sync/channel_manager.h"
#include "sync/data_types.h"

template <typename Algo>
class Sync {
 public:
  ~Sync();
  Sync(const Sync&) = delete;
  Sync<Algo>& operator=(const Sync&) = delete;
  static Sync& instance();
  bool add_channel(const std::string& channel_name,
                   const BaseChannelHolder::SharedPtr& channel);
  bool add_basic_channel(const std::string& channel_name,
                         size_t queue_length = 10);
  void run();
  void start();
  void stop();
  void join();
  void set_data_cb(const DataCb& cb);
  void set_mini_time_interval(TimestampType interval);
  void set_channel_nums(size_t n);
  template <typename MsgType>
  const BaseMsgHolder::SharedPtr& push(const std::string& channel_name,
                                       MsgType&& data, TimestampType t);
  bool has_empty_channel();

 private:
  Sync();
  Algo algo_;
  ChannelType channel_nullptr_;
  ChannelManager channel_mgr_;
  DataCb data_cb_;
  std::thread worker_;
  std::mutex mtx_;
  std::condition_variable cv_;
};

template <typename Algo>
Sync<Algo>::Sync() : algo_(&channel_mgr_) {}

template <typename Algo>
Sync<Algo>::~Sync() {
  stop();
  join();
}

template <typename Algo>
Sync<Algo>& Sync<Algo>::instance() {
  static Sync<Algo> sync;
  return sync;
}

template <typename Algo>
void Sync<Algo>::run() {
  if (!worker_.joinable()) {
    worker_ = std::thread(&Sync::start, this);
  }
}

template <typename Algo>
void Sync<Algo>::start() {
  channel_mgr_.wait_all_channels();
  channel_mgr_.start();

  algo_.sync(data_cb_);
}

template <typename Algo>
void Sync<Algo>::stop() {
  algo_.stop();
  channel_mgr_.stop();
}

template <typename Algo>
void Sync<Algo>::join() {
  if (worker_.joinable()) worker_.join();
}

template <typename Algo>
bool Sync<Algo>::add_channel(const std::string& channel_name,
                             const BaseChannelHolder::SharedPtr& channel) {
  return channel_mgr_.add_channel(channel_name, channel);
}

template <typename Algo>
bool Sync<Algo>::add_basic_channel(const std::string& channel_name,
                                   size_t queue_length) {
  auto ch = std::make_shared<BasicChannelHolder>(channel_name, queue_length);
  return add_channel(channel_name, ch);
}

template <typename Algo>
void Sync<Algo>::set_data_cb(const DataCb& cb) {
  data_cb_ = cb;
}

template <typename Algo>
void Sync<Algo>::set_mini_time_interval(TimestampType interval) {
  algo_.set_mini_time_interval(interval);
}

template <typename Algo>
void Sync<Algo>::set_channel_nums(size_t n) {
  channel_mgr_.set_channel_nums(n);
}

template <typename Algo>
template <typename MsgType>
const BaseMsgHolder::SharedPtr& Sync<Algo>::push(
    const std::string& channel_name, MsgType&& data, TimestampType t) {
  return channel_mgr_.push(channel_name, std::forward<MsgType>(data), t);
}

template <typename Algo>
bool Sync<Algo>::has_empty_channel() {
  return channel_mgr_.has_empty_channel();
}
