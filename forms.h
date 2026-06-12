#ifndef FORMS_H
#define FORMS_H

#include <string>
#include <unordered_map>
#include <vector>

#include "./value.h"

class EvalEnv;

// 特殊形式函数的类型：接收参数列表和当前环境，返回求值结果
using SpecialFormType = ValuePtr(const std::vector<ValuePtr>&, EvalEnv&);

extern const std::unordered_map<std::string, SpecialFormType*> SPECIAL_FORMS;

#endif
