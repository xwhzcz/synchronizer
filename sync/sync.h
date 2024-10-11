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
  const BaseMsgHolder::SharedPtr& push(const std::string& channel_name,
                                       const BaseMsgHolder::SharedPtr& msg);
  template <typename MsgType>
  const BaseMsgHolder::SharedPtr& push(const std::string& channel_name,
                                       const MsgType& data, TimestampType t);
  bool has_empty_channel();

 private:
  Sync();
  Algo algo_;
  ChannelType channel_nullptr_;
  BaseMsgHolder::SharedPtr msg_nullptr_;
  ChannelContainer channels_;
  size_t channel_nums_{0};
  DataCb data_cb_;
  std::thread worker_;
  std::mutex mtx_;
  std::condition_variable cv_;
};

template <typename Algo>
Sync<Algo>::Sync() : algo_(&channels_) {}

template <typename Algo>
Sync<Algo>::~Sync() {
  stop();
  if (worker_.joinable()) worker_.join();
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
  {
    std::unique_lock lk(mtx_);
    cv_.wait(lk, [this]() {
      return (channel_nums_ == 0 && !channels_.empty()) ||
             channels_.size() == channel_nums_;
    });
  }

  for (const auto& [name, channel] : channels_) {
    channel->wait_not_empty();
  }
  for (const auto& [name, channel] : channels_) {
    channel->concurrent_clear();
  }

  algo_.sync(data_cb_);
}

template <typename Algo>
void Sync<Algo>::stop() {
  algo_.stop();
  for (const auto& [name, channel] : channels_) {
    channel->stop();
  }
}

template <typename Algo>
void Sync<Algo>::join() {
  if (worker_.joinable()) worker_.join();
}

template <typename Algo>
bool Sync<Algo>::add_channel(const std::string& channel_name,
                             const BaseChannelHolder::SharedPtr& channel) {
  std::unique_lock lk(mtx_);
  if (channels_.count(channel_name) != 0) return false;
  if (channels_.size() < channel_nums_) {
    channels_[channel_name] = channel;
    cv_.notify_all();
    return true;
  }
  return false;
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
  channel_nums_ = n;
}

template <typename Algo>
const BaseMsgHolder::SharedPtr& Sync<Algo>::push(
    const std::string& channel_name, const BaseMsgHolder::SharedPtr& msg) {
  if (channels_.count(channel_name) == 0 || channels_.size() != channel_nums_)
    return msg_nullptr_;
  return channels_.at(channel_name)->concurrent_push(msg);
}

template <typename Algo>
template <typename MsgType>
const BaseMsgHolder::SharedPtr& Sync<Algo>::push(
    const std::string& channel_name, const MsgType& data, TimestampType t) {
  auto msg =
      std::make_shared<MsgHolder<MsgType>>(std::make_shared<MsgType>(data), t);
  return push(channel_name, msg);
}

template <typename Algo>
bool Sync<Algo>::has_empty_channel() {
  for (const auto& [name, channel] : channels_) {
    if (channel->empty()) return true;
  }
  return false;
}
