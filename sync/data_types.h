/**
 * @file data_types.h
 * @author Wenhao Xin (wenhao.xin@liangdao.ai)
 * @brief
 * @version 0.1
 * @date 2023-08-10
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <climits>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "ros/ros.h"

using TimestampType = uint64_t;
#define TIMESTAMP_MAX ULLONG_MAX

class BaseHolder {
 public:
  virtual ~BaseHolder() = default;
  using SharedPtr = std::shared_ptr<BaseHolder>;
};

class BaseMsgHolder : public BaseHolder {
 public:
  using SharedPtr = std::shared_ptr<BaseMsgHolder>;
  ~BaseMsgHolder() override = default;
  virtual TimestampType time() = 0;
};

template <typename MsgType>
class MsgHolder : public BaseMsgHolder {
 public:
  using SharedPtr = std::shared_ptr<MsgHolder<MsgType>>;
  MsgHolder(const std::shared_ptr<MsgType>& m, TimestampType t)
      : msg_(m), time_(t) {}
  TimestampType time() override { return time_; }
  std::shared_ptr<MsgType>& data() { return msg_; }
  static std::shared_ptr<MsgType>& msg(const BaseMsgHolder::SharedPtr& data) {
    return std::dynamic_pointer_cast<MsgHolder<MsgType>>(data)->data();
  }

 private:
  std::shared_ptr<MsgType> msg_;
  TimestampType time_;
};

class BaseChannelHolder : public BaseHolder {
 public:
  using SharedPtr = std::shared_ptr<BaseChannelHolder>;
  explicit BaseChannelHolder(const std::string& channel_name)
      : channel_name_(channel_name) {}
  virtual const BaseMsgHolder::SharedPtr& push(
      const BaseMsgHolder::SharedPtr& msg) = 0;
  virtual const BaseMsgHolder::SharedPtr& concurrent_push(
      const BaseMsgHolder::SharedPtr& msg) = 0;
  std::string name() const { return channel_name_; }
  std::mutex& mtx() { return mtx_; }
  void lock() { mtx_.lock(); }
  void unlock() { mtx_.unlock(); }
  void stop() {
    running_.store(false);
    notify_all();
  }
  void notify_all() { cv_.notify_all(); }
  virtual const BaseMsgHolder::SharedPtr& at(int pos) = 0;
  virtual const BaseMsgHolder::SharedPtr& concurrent_at(int pos) = 0;
  virtual const BaseMsgHolder::SharedPtr& wait_at(int pos) = 0;
  virtual void erase(int pos) = 0;
  virtual void erase(int begin, int len) = 0;
  virtual void concurrent_erase(int begin, int len) = 0;
  virtual bool empty() = 0;
  virtual void wait_not_empty() = 0;
  virtual void clear() = 0;
  virtual void concurrent_clear() = 0;

 protected:
  std::mutex mtx_;
  std::condition_variable cv_;
  std::string channel_name_;
  std::atomic<bool> running_{true};
};

class BasicChannelHolder : public BaseChannelHolder {
 public:
  using SharedPtr = std::shared_ptr<BasicChannelHolder>;
  explicit BasicChannelHolder(const std::string& channel_name,
                              size_t queue_length = 10)
      : BaseChannelHolder(channel_name), queue_length_(queue_length) {}

  const BaseMsgHolder::SharedPtr& push(
      const BaseMsgHolder::SharedPtr& msg) override {
    if (data_.size() >= queue_length_) return nullptr_;
    data_.emplace_back(msg);
    cv_.notify_all();
    return msg;
  }

  const BaseMsgHolder::SharedPtr& concurrent_push(
      const BaseMsgHolder::SharedPtr& msg) override {
    std::unique_lock lk(mtx_);
    return push(msg);
  }

 private:
  const BaseMsgHolder::SharedPtr& at(int pos) override {
    if (pos >= 0 && pos < static_cast<int64_t>(data_.size())) {
      return data_.at(pos);
    }
    return nullptr_;
  }
  const BaseMsgHolder::SharedPtr& concurrent_at(int pos) override {
    std::unique_lock lk(mtx_);
    return at(pos);
  }
  const BaseMsgHolder::SharedPtr& wait_at(int pos) override {
    std::unique_lock lk(mtx_);
    cv_.wait(lk, [this, pos]() { return !running_.load() || at(pos); });
    if (!running_.load()) {
      return nullptr_;
    }
    return at(pos);
  }
  void erase(int pos) override {
    if (pos >= 0 && pos < static_cast<int64_t>(data_.size()))
      data_.erase(data_.begin() + pos);
  }
  void erase(int begin, int len) override {
    if (begin >= 0 && begin + len <= static_cast<int64_t>(data_.size()))
      data_.erase(data_.begin() + begin, data_.begin() + begin + len);
  }
  void concurrent_erase(int begin, int len) override {
    std::unique_lock lk(mtx_);
    erase(begin, len);
  }
  bool empty() override { return data_.empty(); }
  void wait_not_empty() override {
    std::unique_lock lk(mtx_);
    cv_.wait(lk, [this]() { return !running_.load() || !empty(); });
  }
  void clear() override { data_.clear(); }
  void concurrent_clear() override {
    std::unique_lock lk(mtx_);
    clear();
  }

  BaseMsgHolder::SharedPtr nullptr_;
  std::deque<BaseMsgHolder::SharedPtr> data_;
  size_t queue_length_;
};

template <typename MsgType>
class RosChannelHolder : private BasicChannelHolder {
 public:
  using SharedPtr = std::shared_ptr<RosChannelHolder<MsgType>>;
  RosChannelHolder(const std::string& channel_name,
                   const std::string& topic_name, size_t queue_length = 10)
      : BasicChannelHolder(channel_name, queue_length) {
    ros::NodeHandle nh;
    sub_(nh.subscribe(topic_name, queue_length, on_receive));
  }

 private:
  void on_receive(const std::shared_ptr<MsgType>& msg) {
    concurrent_push(std::make_shared(msg, msg->header.stamp));
  }

  ros::Subscriber sub_;
};

using DataCb = std::function<void(
    const std::unordered_map<std::string, BaseMsgHolder::SharedPtr>&)>;
using ChannelType = BaseChannelHolder::SharedPtr;
using ChannelContainer = std::unordered_map<std::string, ChannelType>;
using ChannelsContainer = std::shared_ptr<ChannelContainer>;
using OutDataType = std::unordered_map<std::string, BaseMsgHolder::SharedPtr>;
