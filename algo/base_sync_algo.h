/**
 * @file basic_sync_algo.h
 * @author Wenhao Xin (wenhao.xin@liangdao.ai)
 * @brief
 * @version 0.1
 * @date 2023-08-11
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <atomic>

#include "sync/data_types.h"

class BaseSyncAlog {
 public:
  using DataCb = std::function<void(
      std::unordered_map<std::string, BaseMsgHolder::SharedPtr>)>;
  virtual ~BaseSyncAlog() = default;
  virtual void sync(const DataCb& data_cb) = 0;
  virtual void stop() { running_.store(false); };
  virtual void set_mini_time_interval(TimestampType interval) = 0;

 protected:
  std::atomic<bool> running_{false};
};
