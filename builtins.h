#ifndef BUILTINS_H
#define BUILTINS_H

#include <string>
#include <unordered_map>

#include "./value.h"

// 全局标志：是否在 REPL 模式（用于控制输出是否着色）
extern bool g_replMode;

// 暴露一个 map，key 是符号名，value 是函数指针
extern const std::unordered_map<std::string, BuiltinFuncType*> BUILTINS;

#endif
