#ifndef VALUE_H
#define VALUE_H

#include "./error.h"

#include<string>
#include<vector>
#include<memory>
#include<optional>

class Value;
class EvalEnv;
using ValuePtr = std::shared_ptr<Value>;
// 函数指针类型定义
using BuiltinFuncType = ValuePtr(const std::vector<ValuePtr>&, EvalEnv&);

// Value基类
class Value {
protected:
    Value() {};
public:
    virtual ~Value() {};
    virtual std::string toString() const = 0;
    //工具函数
    virtual bool isNil() const {
        return false;
    }
    virtual bool isSelfEvaluating() const {
        return false;
    }
    //将列表形式的Value转换为vector
    virtual std::vector<ValuePtr> toVector() const;

    //如果是符号返回符号名，否则返回nullopt
    virtual std::optional<std::string> asSymbol() const {
        return std::nullopt;
    }
    //辅助方法
    virtual bool isNumber() const {
        return false;
    }
    virtual double asNumber() const {
        throw LispError("Not a number.");
    }
    //Lv5特殊形式新增
    virtual bool asBoolean() const {
        throw LispError("Not a boolean.");
    }

};

//布尔类型
class BooleanValue : public Value {
    const bool value;
public:
    explicit BooleanValue(bool value) : value(value) {}
    std::string toString() const override;
    bool isSelfEvaluating() const override {
        return true;
    }

    bool asBoolean() const override {
        return value;
    }
};
//数值类型
class NumericValue : public Value {
    const double value;
public:
    explicit NumericValue(double value) : value{value} {}
    std::string toString() const override;
    bool isSelfEvaluating() const override {
        return true;
    }
    //
    bool isNumber() const override {
        return true;
    }
    double asNumber() const override {
        return value;
    }
};
//字符串类型
class StringValue : public Value {
    const std::string value;
public:
    explicit StringValue(std::string value) : value(value) {}
    std::string toString() const override;
    bool isSelfEvaluating() const override {
        return true;
    }
};
//空表
class NilValue : public Value {
public:
    NilValue() {}
    std::string toString() const override;
    bool isNil() const override {
        return true;
    }
    virtual std::vector<ValuePtr> toVector() const override;
};
//符号类型
class SymbolValue : public Value {
    const std::string value;
public:
    explicit SymbolValue(std::string value) : value(value) {}
    std::string toString() const override;
    virtual std::optional<std::string> asSymbol() const override;
};
//对子类型
class PairValue : public Value {
protected:
    const ValuePtr car;
    const ValuePtr cdr;
public:
    PairValue(ValuePtr car, ValuePtr cdr) : car(car), cdr(cdr) {}
    std::string toString() const override;
    std::string toStringTail() const;
    virtual std::vector<ValuePtr> toVector() const override;
    //获取右半部分，供evalList使用
    ValuePtr getCdr() const {
        return cdr;
    }

    ValuePtr getCar() const {
        return car;
    }
};

// BuiltinProcValue 类
class BuiltinProcValue : public Value {
    BuiltinFuncType* func;
public:
    BuiltinProcValue(BuiltinFuncType* func) : func{func} {}
    std::string toString() const override {
        return "#<procedure>";
    }
    bool isSelfEvaluating() const override {
        return true;
    }
    // 供 apply 调用，执行内部函数指针
    ValuePtr call(const std::vector<ValuePtr>& args, EvalEnv& env) const {
        return func(args, env);
    }
};


//LambdaValue类
class LambdaValue : public Value {
    std::vector<std::string> params;  //形参列表
    std::vector<ValuePtr> body;       // 函数体
    std::shared_ptr<EvalEnv> closure;  // 闭包环境，捕获定义时的环境
public:
    LambdaValue(std::vector<std::string> params,
               std::vector<ValuePtr> body,
                std::shared_ptr<EvalEnv> closure)
        : params(params), body(body), closure(closure) {}
    std::string toString() const override {
        return "#<procedure>";
    }
    bool isSelfEvaluating() const override {
        return true;
    }

    // 调用lambda函数，传入实参列表
    ValuePtr apply(const std::vector<ValuePtr>& args) const;
};
#endif
