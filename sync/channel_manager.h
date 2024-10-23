/**
 * @file channel_manager.h
 * @author Wenhao Xin (wenhao.xin@liangdao.ai)
 * @brief 监控 channel 是否活跃
 * @version 0.1
 * @date 2024-10-22
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once
#include <atomic>
#include <thread>

#include "sync/data_types.h"

class ChannelManager {
 public:
  ChannelManager() = default;
  ~ChannelManager();
  inline void set_channel_nums(size_t n);
  bool add_channel(const std::string& channel_name,
                   const BaseChannelHolder::SharedPtr& channel);
  void wait_all_channels();
  ChannelContainer get_alive_channels();
  void check_alive();
  inline bool bad_alive();
  inline void init_bad_alive();
  const BaseMsgHolder::SharedPtr& push(const std::string& channel_name,
                                       const BaseMsgHolder::SharedPtr& msg);
  template <typename MsgType>
  const BaseMsgHolder::SharedPtr& push(const std::string& channel_name,
                                       MsgType&& data, TimestampType t) {
    using type = typename std::remove_reference<MsgType>::type;
    auto msg = std::make_shared<MsgHolder<type>>(std::forward<type>(data), t);
    return push(channel_name, msg);
  }
  bool has_empty_channel();
  void start();
  void run();
  void stop();
  inline void join();

 private:
  ChannelContainer channels_;
  BaseMsgHolder::SharedPtr msg_nullptr_;
  size_t channel_nums_{0};
  std::mutex channels_mtx_;
  std::condition_variable channels_cv_;
  std::atomic<bool> bad_alive_{false};
  ChannelContainer alive_channels_;
  std::mutex alive_channels_mtx_;
  std::condition_variable alive_channels_cv_;
  std::thread worker_;
  std::atomic<bool> running_{true};
};

inline void ChannelManager::set_channel_nums(size_t n) { channel_nums_ = n; }

inline bool ChannelManager::bad_alive() { return bad_alive_; }

inline void ChannelManager::init_bad_alive() { bad_alive_ = false; }

inline void ChannelManager::join() {
  if (worker_.joinable()) worker_.join();
}