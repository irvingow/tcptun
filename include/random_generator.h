//
// Created by lwj on 2019/9/22.
//

#ifndef UDPTUN_RANDOM_GENERATOR_H
#define UDPTUN_RANDOM_GENERATOR_H

#include "noncopyable.h"
#include <cstdint>

///单例模式实现
class RandomNumberGenerator : public noncopyable {
 public:
  static RandomNumberGenerator *GetInstance();
  int32_t GetRandomNumber(uint32_t &random_number);
  int32_t GetRandomNumberNonZero(uint32_t &random_number);

 protected:
  struct Obj_Creator {
    Obj_Creator();
  };
  static Obj_Creator obj_creator_;
  RandomNumberGenerator();
  ~RandomNumberGenerator();
  int32_t random_number_fd_;
};

#endif //UDPTUN_RANDOM_GENERATOR_H
