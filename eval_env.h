#ifndef EVAL_ENV_H
#define EVAL_ENV_H

#include "value.h"

#include<string>
#include <vector>
#include<unordered_map>
#include<memory>

class EvalEnv : public std::enable_shared_from_this<EvalEnv> {
private:
    std::shared_ptr<EvalEnv> parent;  // 父环境（用于变量查找链）
    std::unordered_map<std::string, ValuePtr> symbolTable;  // 当前环境的符号表

    // 私有构造函数，外部通过工厂方法创建
    EvalEnv() = default;
    explicit EvalEnv(std::shared_ptr<EvalEnv> parent) : parent{parent} {}

    std::vector<ValuePtr> evalList(ValuePtr expr);  // 对列表每个元素求值
    ValuePtr apply(ValuePtr proc, std::vector<ValuePtr> args);  // 调用过程

public:
    // 创建全局环境（含所有内置过程）
    static std::shared_ptr<EvalEnv> createGlobal();
    // 以当前环境为父环境创建子环境（用于 lambda 调用）
    std::shared_ptr<EvalEnv> createChild();

    // 在当前环境定义变量
    void defineVar(const std::string& name, ValuePtr value);
    // 沿环境链查找变量
    ValuePtr lookupVar(const std::string& name);

    ValuePtr eval(ValuePtr expr);
};
#endif
