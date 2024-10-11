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

class Cb {
 public:
  void data_cb(const std::unordered_map<std::string, BaseMsgHolder::SharedPtr>&
                   synced_data) {
    for (const auto& [name, m] : synced_data) {
      std::cout << "Data calllback receive data: " << name << std::endl;
      auto data = synced_data.at(name);
      if (name == ch_name1) {
        auto msg = MsgHolder<std::string>::msg(data);
        std::cout << name << " " << *msg << " " << data->time() << std::endl;
      } else {
        auto msg = MsgHolder<int>::msg(data);
        std::cout << name << " " << *msg << " " << data->time() << std::endl;
      }
    }
    std::cout << "-----------------------------------" << std::endl;
  }
};

void add_string_msg(const std::string& ch_name) {
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  int count = 0;
  while (true) {
    ++count;
    std::string data = ch_name + std::to_string(count);
    auto msg = std::make_shared<MsgHolder<std::string>>(
        std::make_shared<std::string>(data),
        std::chrono::system_clock::now().time_since_epoch().count() -
            2000000000);
    Sync<ApproximateTimeAlgo>::instance().push(ch_name, msg);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

void add_int_msg(const std::string& ch_name) {
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  int count = 0;
  while (true) {
    ++count;
    Sync<ApproximateTimeAlgo>::instance().push(
        ch_name, count,
        std::chrono::system_clock::now().time_since_epoch().count());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

int main(int argc, char** argv) {
  ros::init(argc, argv, "sync");
  ros::NodeHandle nh;

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
  std::cout << "sync has stopped" << std::endl;
  Sync<ApproximateTimeAlgo>::instance().join();
  Sync<ApproximateTimeAlgo>::instance().join();
  t1.join();
  t2.join();

  return 0;
}