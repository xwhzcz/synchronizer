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
#include "sync/data_types.h"

class ChannelManager {
 public:
  ChannelManager() {}

  void set_channel_nums(size_t n) { channel_nums_ = n; }

  bool add_channel(const std::string& channel_name,
                   const BaseChannelHolder::SharedPtr& channel) {
    std::unique_lock lk(

    );
    if (channels_.count(channel_name) != 0) return false;
    if (channels_.size() < channel_nums_) {
      channels_[channel_name] = channel;
      channels_cv_.notify_one();
      return true;
    }
    return false;
  }

  void wait_all_channels() {
    std::unique_lock lk(channels_mtx_);
    channels_cv_.wait(lk, [this]() {
      return (channel_nums_ == 0 && !channels_.empty()) ||
             channels_.size() == channel_nums_;
    });

    for (const auto& [name, channel] : channels_) {
      channel->wait_not_empty();
    }
    for (const auto& [name, channel] : channels_) {
      channel->concurrent_clear();
    }
  }

  ChannelContainer get_active_channels() {
    ChannelContainer active_channels;
    active_channels = channels_;
    return active_channels;
  }

  const BaseMsgHolder::SharedPtr& push(const std::string& channel_name,
                                       const BaseMsgHolder::SharedPtr& msg) {
    if (channels_.count(channel_name) == 0) return msg_nullptr_;
    return channels_.at(channel_name)->concurrent_push(msg);
  }

  template <typename MsgType>
  const BaseMsgHolder::SharedPtr& push(const std::string& channel_name,
                                       MsgType&& data, TimestampType t) {
    using type = typename std::remove_reference<MsgType>::type;
    auto msg = std::make_shared<MsgHolder<type>>(std::forward<type>(data), t);
    return push(channel_name, msg);
  }

  bool has_empty_channel() {
    for (const auto& [name, channel] : channels_) {
      if (channel->empty()) return true;
    }
    return false;
  }

  void start() {}

  void stop() {
    for (const auto& [name, channel] : channels_) {
      channel->stop();
    }
  }

 private:
  ChannelContainer channels_;
  BaseMsgHolder::SharedPtr msg_nullptr_;
  size_t channel_nums_{0};
  std::mutex channels_mtx_;
  std::condition_variable channels_cv_;
};