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

#include "sync/channel_manager.h"

ChannelManager::~ChannelManager() {
  stop();
  join();
}

bool ChannelManager::add_channel(const std::string& channel_name,
                                 const BaseChannelHolder::SharedPtr& channel) {
  std::unique_lock<std::mutex> lk(channels_mtx_);
  if (channels_.count(channel_name) != 0) return false;
  if (channel_nums_ == 0 || channels_.size() < channel_nums_) {
    channels_[channel_name] = channel;
    channels_cv_.notify_one();
    return true;
  }
  return false;
}

void ChannelManager::wait_all_channels() {
  std::unique_lock<std::mutex> lk(channels_mtx_);
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

ChannelContainer ChannelManager::get_alive_channels() {
  ChannelContainer local_alive_channels;
  {
    std::unique_lock<std::mutex> channels_lk(channels_mtx_);
    for (const auto& [name, channel] : channels_) {
      if (channel->is_alive()) {
        local_alive_channels.insert(std::make_pair(name, channel));
      }
    }
  }
  if (local_alive_channels != alive_channels_) {
    std::unique_lock<std::mutex> alive_channels_lk(alive_channels_mtx_);
    alive_channels_ = local_alive_channels;
  }
  return local_alive_channels;  // 返回值优化
}

void ChannelManager::check_alive() {
  std::unique_lock<std::mutex> alive_channels_lk(alive_channels_mtx_);
  for (const auto& [name, channel] : alive_channels_) {
    if (!channel->is_alive()) {
      bad_alive_ = true;
      channel->notify_all();
    }
  }
}

const BaseMsgHolder::SharedPtr& ChannelManager::push(
    const std::string& channel_name, const BaseMsgHolder::SharedPtr& msg) {
  if (channels_.count(channel_name) == 0) return msg_nullptr_;
  return channels_.at(channel_name)->concurrent_push(msg);
}

bool ChannelManager::has_empty_channel() {
  for (const auto& [name, channel] : channels_) {
    if (channel->empty()) return true;
  }
  return false;
}

void ChannelManager::start() {
  if (!worker_.joinable()) {
    worker_ = std::thread(&ChannelManager::run, this);
  }
}

void ChannelManager::run() {
  while (running_.load()) {
    check_alive();
    std::this_thread::sleep_for(std::chrono::milliseconds{200});
  }
}

void ChannelManager::stop() {
  for (const auto& [name, channel] : channels_) {
    channel->stop();
  }
  running_ = false;
  join();
}
