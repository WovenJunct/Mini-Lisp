#include"value.h"
#include<string>
#include<iomanip>
#include<iostream>
#include<sstream>
#include"error.h"
#include "eval_env.h"

// Value基类的工具函数
std::vector<ValuePtr> Value::toVector() const {
    throw LispError("Not a list");
}

//布尔类型外部表示
std::string BooleanValue:: toString() const {
    return (value == true) ? "#t" : "#f";
}
// 数值类型外部表示
/* ss << value调用标准库的
* std::ostream& operator<<(std::ostream& os, double val);
*/
std::string NumericValue:: toString() const {
    std::ostringstream ss;
    ss << value; 
    return ss.str();
}
// 字符串类型外部表示
std::string StringValue::toString() const {
    std::ostringstream ss;
    ss << std::quoted(value);
    return ss.str();
}
// 空表外部表示
std::string NilValue::toString() const {
    return "()";
}
//NilValue空列表，返回空vector
std::vector<ValuePtr> NilValue::toVector() const {
    return {};
}
// 符号类型外部表示
std::string SymbolValue::toString() const {
    return value;
}
std::optional<std::string> SymbolValue::asSymbol() const {
    return value;
}

// 对子类型外部表示
//typedid判断value类型
//使用辅助函数避免递归过程汇中多个左括号出现
std::string PairValue::toString() const {
    return "(" + toStringTail();
}
std::string PairValue::toStringTail() const {
    std::string result = car->toString();
    if (typeid(*cdr) == typeid(NilValue)) {
        result += ")";
    } else if (typeid(*cdr) == typeid(PairValue)) {
        result += " ";
        auto& pair = static_cast<const PairValue&>(*cdr);
        result += pair.toStringTail();
    } else {
        result += " . ";
        result += cdr->toString();
        result += ")";
    }
    return result;
}

//PairValue 遍历整个链表收集元素
std::vector<ValuePtr> PairValue::toVector() const {
    std::vector<ValuePtr> result;
    const PairValue* cur = this;
    while (true) {
        result.push_back(cur->car);
        if (typeid(*cur->cdr) == typeid(NilValue)) {
            break;
        } else if (typeid(*cur->cdr) == typeid(PairValue)) {
            cur = static_cast<const PairValue*>(cur->cdr.get());
        } else {
            throw LispError("Not a proper list");
        }
    }
    return result;
}

ValuePtr LambdaValue::apply(const std::vector<ValuePtr>& args) const {
    // 检查参数数量
    if (args.size() != params.size()) {
        throw LispError("Argument count mismatch:expected " +
                        std::to_string(params.size()) + " but got " +
                        std::to_string(args.size()) + ".");
    }
    // 创建新的环境，父环境为闭包环境
    auto env = closure->createChild();
    // 将实参绑定到形参
    for (size_t i = 0; i < params.size(); i++) {
        env->defineVar(params[i], args[i]);
    }
    // 依次求值函数体的每个表达式，返回最后一个表达式的值
    ValuePtr result = std::make_shared<NilValue>();
    for (auto& expr : body) {
        result = env->eval(expr);
    }
    return result;
}
