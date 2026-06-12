#ifndef BUILTINS_H
#define BUILTINS_H

#include <string>
#include <unordered_map>

#include "./value.h"

// 暴露一个 map，key 是符号名，value 是函数指针
extern const std::unordered_map<std::string, BuiltinFuncType*> BUILTINS;

#endif
