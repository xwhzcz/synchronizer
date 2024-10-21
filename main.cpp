/**
 * @file main.cpp
 * @author Wenhao Xin (wenhao.xin@liangdao.ai)
 * @brief
 * @version 0.1
 * @date 2023-08-09
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <chrono>
#include <functional>
#include <thread>

#include "algo/approximate_time_algo.h"
#include "sync/sync.h"

const std::string ch_name1 = "foo";
const std::string ch_name2 = "bar";
bool stopped = false;

#include <cstring>
#include <memory>

class MyString {
 public:
  MyString(const char* data = nullptr) : capacity_(0), data_(nullptr) {
    if (data) {
      capacity_ = strlen(data) + 1;
      data_ = new char[capacity_];
      memcpy(data_, data, capacity_);
    }
  }
  MyString(const MyString& other) {
    capacity_ = other.size() + 1;
    data_ = new char[capacity_];
    memcpy(data_, other.data(), capacity_);
    // std::cout << "MyString 拷贝构造" << std::endl;
  }
  MyString(MyString&& other) {
    capacity_ = other.capacity();
    data_ = other.data();
    other = nullptr;
    // std::cout << "MyString 移动构造" << std::endl;
  }
  ~MyString() { delete data_; }
  MyString& operator=(const char* data) {
    if (data && data != data_) {
      delete data_;
      capacity_ = strlen(data) + 1;
      data_ = new char[capacity_];
      memcpy(data_, data, capacity_);
    } else if (data == nullptr) {
      capacity_ = 0;
      data_ = nullptr;
    }
    return *this;
  }
  MyString& operator=(const MyString& other) {
    delete data_;
    capacity_ = other.capacity();
    data_ = new char[capacity_];
    memcpy(data_, other.data(), capacity_);
    return *this;
  }
  MyString& operator=(MyString&& other) {
    delete data_;
    capacity_ = other.capacity();
    data_ = other.data();
    other = nullptr;
    return *this;
  }
  MyString& operator+(const MyString& other) {
    if (size() + other.size() + 1 > capacity()) {
      while (size() + other.size() + 1 > capacity()) {
        capacity_ *= 2;
      }
      char* new_data = new char[capacity_];
      memcpy(new_data, data_, size());
      memcpy(new_data + size(), other.data(), other.size() + 1);
      data_ = new_data;
    } else {
      memcpy(data_ + size(), other.data(), other.size() + 1);
    }
    return *this;
  }
  size_t size() const {
    if (data_ == nullptr) {
      return 0;
    }
    return strlen(data_);
  }
  char* data() const { return data_; }

  size_t capacity() const {
    if (data_ != nullptr)
      return capacity_;
    else
      return 0;
  }

 private:
  char* data_;
  size_t capacity_;
};

class Cb {
 public:
  void data_cb(const std::unordered_map<std::string, BaseMsgHolder::SharedPtr>&
                   synced_data) {
    for (const auto& [name, m] : synced_data) {
      std::cout << "Data calllback receive data: " << name << std::endl;
      auto data = synced_data.at(name);
      if (name == ch_name1) {
        auto& msg = MsgHolder<MyString>::msg(data);
        std::cout << name << " " << msg.data() << " " << data->time()
                  << std::endl;
      } else {
        auto msg = MsgHolder<int>::msg(data);
        std::cout << name << " " << msg << " " << data->time() << std::endl;
      }
    }
    std::cout << "-----------------------------------" << std::endl;
  }
};

void add_string_msg(const std::string& ch_name) {
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  int count = 0;
  while (!stopped) {
    ++count;
    MyString data = "channel_1";
    Sync<ApproximateTimeAlgo>::instance().push(
        ch_name, std::move(data),
        std::chrono::system_clock::now().time_since_epoch().count() -
            2000000000);  // 模拟消息延迟，验证在延迟情况下的同步准确性
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

void add_int_msg(const std::string& ch_name) {
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  int count = 0;
  while (!stopped) {
    ++count;
    Sync<ApproximateTimeAlgo>::instance().push(
        ch_name, count,
        std::chrono::system_clock::now().time_since_epoch().count());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

int main(int argc, char** argv) {
  // 设置回调函数
  Cb cb;
  Sync<ApproximateTimeAlgo>::instance().set_data_cb(
      std::bind(&Cb::data_cb, cb, std::placeholders::_1));

  // 设置channel数量，注意此项为必设项，channel实际数量等于设置值时才会开始同步
  Sync<ApproximateTimeAlgo>::instance().set_channel_nums(2);
  Sync<ApproximateTimeAlgo>::instance().run();

  // 添加channel
  auto ch1 = std::make_shared<BasicChannelHolder>(ch_name1, 50);
  Sync<ApproximateTimeAlgo>::instance().add_channel(ch1->name(), ch1);
  std::this_thread::sleep_for(std::chrono::seconds(5));
  // auto ch2 = std::make_shared<BasicChannelHolder>(ch_name2, 50);
  Sync<ApproximateTimeAlgo>::instance().add_basic_channel(ch_name2, 50);

  // 添加数据
  auto t1 = std::thread(add_string_msg, ch_name1);
  std::this_thread::sleep_for(std::chrono::seconds(5));
  auto t2 = std::thread(add_int_msg, ch_name2);

  std::this_thread::sleep_for(std::chrono::seconds(10));
  Sync<ApproximateTimeAlgo>::instance().stop();
  stopped = true;
  std::cout << "sync has stopped" << std::endl;
  Sync<ApproximateTimeAlgo>::instance().join();
  t1.join();
  t2.join();

  return 0;
}