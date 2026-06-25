#include "./builtins.h"

#include <cmath>
#include <functional>
#include <iostream>

#include "./error.h"
#include "./eval_env.h"
#include "highlight.h"

// 辅助函数：调用任意过程
static ValuePtr callProc(ValuePtr proc, std::vector<ValuePtr> args,
                         EvalEnv& env) {
    if (typeid(*proc) == typeid(BuiltinProcValue))
        return static_cast<BuiltinProcValue&>(*proc).call(args, env);
    if (typeid(*proc) == typeid(LambdaValue))
        return static_cast<LambdaValue&>(*proc).apply(args);
    throw LispError("Not a procedure.");
}

// ============ 核心 IO 库 ============

ValuePtr print(const std::vector<ValuePtr>& params, EvalEnv& env) {
    for (auto& p : params)
        std::cout << colorize_output(p->toString()) << std::endl;
    return std::make_shared<NilValue>();
}

ValuePtr display(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("display requires 1 argument.");
    if (typeid(*params[0]) == typeid(StringValue)) {
        std::string s = params[0]->toString();
        std::cout << s.substr(1, s.size() - 2);
    } else {
        std::cout << colorize_output(params[0]->toString());
    }
    return std::make_shared<NilValue>();
}

ValuePtr displayln(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("displayln requires 1 argument.");
    if (typeid(*params[0]) == typeid(StringValue)) {
        std::string s = params[0]->toString();
        std::cout << s.substr(1, s.size() - 2) << std::endl;
    } else {
        std::cout << colorize_output(params[0]->toString()) << std::endl;
    }
    return std::make_shared<NilValue>();
}

ValuePtr newline(const std::vector<ValuePtr>& params, EvalEnv& env) {
    std::cout << std::endl;
    return std::make_shared<NilValue>();
}

ValuePtr exit_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.empty()) std::exit(0);
    std::exit((int)params[0]->asNumber());
}

ValuePtr error_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.empty()) throw LispError("Error.");
    throw LispError(params[0]->toString());
}

ValuePtr eval_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("eval requires 1 argument.");
    return env.eval(params[0]);
}

ValuePtr apply_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() < 2)
        throw LispError("apply requires at least 2 arguments.");
    ValuePtr proc = params[0];
    std::vector<ValuePtr> args;
    for (size_t i = 1; i < params.size() - 1; i++) args.push_back(params[i]);
    auto last = params.back()->toVector();
    for (auto& a : last) args.push_back(a);
    return callProc(proc, args, env);
}

// ============ 算术运算库 ============

ValuePtr add(const std::vector<ValuePtr>& params, EvalEnv& env) {
    double result = 0.0;
    for (auto& p : params) {
        if (!p->isNumber()) throw LispError("Cannot add a non-numeric value.");
        result += p->asNumber();
    }
    return std::make_shared<NumericValue>(result);
}

ValuePtr sub(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.empty()) throw LispError("- requires at least 1 argument.");
    if (params.size() == 1)
        return std::make_shared<NumericValue>(-params[0]->asNumber());
    double result = params[0]->asNumber();
    for (size_t i = 1; i < params.size(); i++) result -= params[i]->asNumber();
    return std::make_shared<NumericValue>(result);
}

ValuePtr mul(const std::vector<ValuePtr>& params, EvalEnv& env) {
    double result = 1.0;
    for (auto& p : params) {
        if (!p->isNumber())
            throw LispError("Cannot multiply a non-numeric value.");
        result *= p->asNumber();
    }
    return std::make_shared<NumericValue>(result);
}

ValuePtr div(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() < 1) throw LispError("/ requires at least 1 argument.");
    if (params.size() == 1) {
        if (params[0]->asNumber() == 0) throw LispError("Division by zero.");
        return std::make_shared<NumericValue>(1.0 / params[0]->asNumber());
    }
    double result = params[0]->asNumber();
    for (size_t i = 1; i < params.size(); i++) {
        if (params[i]->asNumber() == 0) throw LispError("Division by zero.");
        result /= params[i]->asNumber();
    }
    return std::make_shared<NumericValue>(result);
}

ValuePtr abs_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("abs requires 1 argument.");
    return std::make_shared<NumericValue>(std::abs(params[0]->asNumber()));
}

ValuePtr expt_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 2) throw LispError("expt requires 2 arguments.");
    return std::make_shared<NumericValue>(
        std::pow(params[0]->asNumber(), params[1]->asNumber()));
}

ValuePtr quotient(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 2) throw LispError("quotient requires 2 arguments.");
    double a = params[0]->asNumber(), b = params[1]->asNumber();
    if (b == 0) throw LispError("Division by zero.");
    return std::make_shared<NumericValue>((int)(a / b));
}

ValuePtr remainder_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 2) throw LispError("remainder requires 2 arguments.");
    double a = params[0]->asNumber(), b = params[1]->asNumber();
    if (b == 0) throw LispError("Division by zero.");
    return std::make_shared<NumericValue>(std::fmod(a, b));
}

ValuePtr modulo_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 2) throw LispError("modulo requires 2 arguments.");
    double a = params[0]->asNumber(), b = params[1]->asNumber();
    if (b == 0) throw LispError("Division by zero.");
    double result = std::fmod(a, b);
    // 结果符号与除数相同
    if ((result > 0 && b < 0) || (result < 0 && b > 0)) result += b;
    return std::make_shared<NumericValue>(result);
}

// ============ 比较库 ============

ValuePtr eq_num(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() < 2) throw LispError("= requires at least 2 arguments.");
    double first = params[0]->asNumber();
    for (size_t i = 1; i < params.size(); i++)
        if (params[i]->asNumber() != first)
            return std::make_shared<BooleanValue>(false);
    return std::make_shared<BooleanValue>(true);
}

ValuePtr less(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() < 2) throw LispError("< requires at least 2 arguments.");
    for (size_t i = 0; i + 1 < params.size(); i++)
        if (params[i]->asNumber() >= params[i + 1]->asNumber())
            return std::make_shared<BooleanValue>(false);
    return std::make_shared<BooleanValue>(true);
}

ValuePtr greater(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() < 2) throw LispError("> requires at least 2 arguments.");
    for (size_t i = 0; i + 1 < params.size(); i++)
        if (params[i]->asNumber() <= params[i + 1]->asNumber())
            return std::make_shared<BooleanValue>(false);
    return std::make_shared<BooleanValue>(true);
}

ValuePtr leq(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() < 2) throw LispError("<= requires at least 2 arguments.");
    for (size_t i = 0; i + 1 < params.size(); i++)
        if (params[i]->asNumber() > params[i + 1]->asNumber())
            return std::make_shared<BooleanValue>(false);
    return std::make_shared<BooleanValue>(true);
}

ValuePtr geq(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() < 2) throw LispError(">= requires at least 2 arguments.");
    for (size_t i = 0; i + 1 < params.size(); i++)
        if (params[i]->asNumber() < params[i + 1]->asNumber())
            return std::make_shared<BooleanValue>(false);
    return std::make_shared<BooleanValue>(true);
}

ValuePtr even_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("even? requires 1 argument.");
    return std::make_shared<BooleanValue>((int)params[0]->asNumber() % 2 == 0);
}

ValuePtr odd_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("odd? requires 1 argument.");
    return std::make_shared<BooleanValue>((int)params[0]->asNumber() % 2 != 0);
}

ValuePtr zero_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("zero? requires 1 argument.");
    return std::make_shared<BooleanValue>(params[0]->asNumber() == 0);
}

ValuePtr not_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("not requires 1 argument.");
    return std::make_shared<BooleanValue>(
        typeid(*params[0]) == typeid(BooleanValue) && !params[0]->asBoolean());
}

// ============ 类型检查库 ============

ValuePtr is_boolean(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("boolean? requires 1 argument.");
    return std::make_shared<BooleanValue>(typeid(*params[0]) ==
                                          typeid(BooleanValue));
}

ValuePtr is_number(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("number? requires 1 argument.");
    return std::make_shared<BooleanValue>(params[0]->isNumber());
}

ValuePtr is_integer(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("integer? requires 1 argument.");
    if (!params[0]->isNumber()) return std::make_shared<BooleanValue>(false);
    double v = params[0]->asNumber();
    return std::make_shared<BooleanValue>(v == (int)v);
}

ValuePtr is_string(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("string? requires 1 argument.");
    return std::make_shared<BooleanValue>(typeid(*params[0]) ==
                                          typeid(StringValue));
}

ValuePtr is_symbol(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("symbol? requires 1 argument.");
    return std::make_shared<BooleanValue>(typeid(*params[0]) ==
                                          typeid(SymbolValue));
}

ValuePtr is_null(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("null? requires 1 argument.");
    return std::make_shared<BooleanValue>(params[0]->isNil());
}

ValuePtr is_pair(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("pair? requires 1 argument.");
    return std::make_shared<BooleanValue>(typeid(*params[0]) ==
                                          typeid(PairValue));
}

ValuePtr is_list(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("list? requires 1 argument.");
    ValuePtr cur = params[0];
    while (!cur->isNil()) {
        if (typeid(*cur) != typeid(PairValue))
            return std::make_shared<BooleanValue>(false);
        cur = static_cast<PairValue&>(*cur).getCdr();
    }
    return std::make_shared<BooleanValue>(true);
}

ValuePtr is_procedure(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("procedure? requires 1 argument.");
    return std::make_shared<BooleanValue>(
        typeid(*params[0]) == typeid(BuiltinProcValue) ||
        typeid(*params[0]) == typeid(LambdaValue));
}

ValuePtr is_atom(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("atom? requires 1 argument.");
    auto& v = *params[0];
    return std::make_shared<BooleanValue>(
        typeid(v) == typeid(BooleanValue) ||
        typeid(v) == typeid(NumericValue) || typeid(v) == typeid(StringValue) ||
        typeid(v) == typeid(SymbolValue) || typeid(v) == typeid(NilValue));
}

// ============ 对子与列表操作库 ============

ValuePtr car_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("car requires 1 argument.");
    if (typeid(*params[0]) != typeid(PairValue))
        throw LispError("car requires a pair.");
    return static_cast<PairValue&>(*params[0]).getCar();
}

ValuePtr cdr_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("cdr requires 1 argument.");
    if (typeid(*params[0]) != typeid(PairValue))
        throw LispError("cdr requires a pair.");
    return static_cast<PairValue&>(*params[0]).getCdr();
}

ValuePtr cons_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 2) throw LispError("cons requires 2 arguments.");
    return std::make_shared<PairValue>(params[0], params[1]);
}

ValuePtr length_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 1) throw LispError("length requires 1 argument.");
    return std::make_shared<NumericValue>((double)params[0]->toVector().size());
}

ValuePtr list_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    ValuePtr result = std::make_shared<NilValue>();
    for (int i = (int)params.size() - 1; i >= 0; i--)
        result = std::make_shared<PairValue>(params[i], result);
    return result;
}

ValuePtr append_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.empty()) return std::make_shared<NilValue>();
    std::vector<ValuePtr> result;
    for (size_t i = 0; i < params.size() - 1; i++) {
        if (!params[i]->isNil()) {
            auto v = params[i]->toVector();
            result.insert(result.end(), v.begin(), v.end());
        }
    }
    ValuePtr tail = params.back();
    for (int i = (int)result.size() - 1; i >= 0; i--)
        tail = std::make_shared<PairValue>(result[i], tail);
    return tail;
}

ValuePtr map_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 2) throw LispError("map requires 2 arguments.");
    ValuePtr proc = params[0];
    auto lst = params[1]->toVector();
    std::vector<ValuePtr> result;
    for (auto& item : lst) result.push_back(callProc(proc, {item}, env));
    ValuePtr tail = std::make_shared<NilValue>();
    for (int i = (int)result.size() - 1; i >= 0; i--)
        tail = std::make_shared<PairValue>(result[i], tail);
    return tail;
}

ValuePtr filter_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 2) throw LispError("filter requires 2 arguments.");
    ValuePtr proc = params[0];
    auto lst = params[1]->toVector();
    std::vector<ValuePtr> result;
    for (auto& item : lst) {
        ValuePtr res = callProc(proc, {item}, env);
        if (!(typeid(*res) == typeid(BooleanValue) && !res->asBoolean()))
            result.push_back(item);
    }
    ValuePtr tail = std::make_shared<NilValue>();
    for (int i = (int)result.size() - 1; i >= 0; i--)
        tail = std::make_shared<PairValue>(result[i], tail);
    return tail;
}

ValuePtr reduce_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 2) throw LispError("reduce requires 2 arguments.");
    ValuePtr proc = params[0];
    auto lst = params[1]->toVector();
    if (lst.empty()) throw LispError("reduce: list cannot be empty.");
    if (lst.size() == 1) return lst[0];
    ValuePtr tail = lst.back();
    for (int i = (int)lst.size() - 2; i >= 0; i--)
        tail = callProc(proc, {lst[i], tail}, env);
    return tail;
}

ValuePtr eq_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 2) throw LispError("eq? requires 2 arguments.");
    auto& a = params[0];
    auto& b = params[1];
    if (typeid(*a) != typeid(*b)) return std::make_shared<BooleanValue>(false);
    if (typeid(*a) == typeid(StringValue) || typeid(*a) == typeid(PairValue))
        return std::make_shared<BooleanValue>(a.get() == b.get());
    return std::make_shared<BooleanValue>(a->toString() == b->toString());
}

ValuePtr equal_proc(const std::vector<ValuePtr>& params, EvalEnv& env) {
    if (params.size() != 2) throw LispError("equal? requires 2 arguments.");
    std::function<bool(ValuePtr, ValuePtr)> equal = [&](ValuePtr a,
                                                        ValuePtr b) -> bool {
        if (typeid(*a) != typeid(*b)) return false;
        if (typeid(*a) == typeid(PairValue)) {
            auto& pa = static_cast<PairValue&>(*a);
            auto& pb = static_cast<PairValue&>(*b);
            return equal(pa.getCar(), pb.getCar()) &&
                   equal(pa.getCdr(), pb.getCdr());
        }
        return a->toString() == b->toString();
    };
    return std::make_shared<BooleanValue>(equal(params[0], params[1]));
}

// ============ 内置过程表 ============

const std::unordered_map<std::string, BuiltinFuncType*> BUILTINS = {
    // 核心 IO
    {"print", print},
    {"display", display},
    {"displayln", displayln},
    {"newline", newline},
    {"exit", exit_proc},
    {"error", error_proc},
    {"eval", eval_proc},
    {"apply", apply_proc},
    // 算术
    {"+", add},
    {"-", sub},
    {"*", mul},
    {"/", div},
    {"abs", abs_proc},
    {"expt", expt_proc},
    {"quotient", quotient},
    {"remainder", remainder_proc},
    {"modulo", modulo_proc},
    // 比较
    {"=", eq_num},
    {"<", less},
    {">", greater},
    {"<=", leq},
    {">=", geq},
    {"even?", even_proc},
    {"odd?", odd_proc},
    {"zero?", zero_proc},
    {"not", not_proc},
    // 类型检查
    {"boolean?", is_boolean},
    {"number?", is_number},
    {"integer?", is_integer},
    {"string?", is_string},
    {"symbol?", is_symbol},
    {"null?", is_null},
    {"pair?", is_pair},
    {"list?", is_list},
    {"procedure?", is_procedure},
    {"atom?", is_atom},
    // 列表操作
    {"car", car_proc},
    {"cdr", cdr_proc},
    {"cons", cons_proc},
    {"length", length_proc},
    {"list", list_proc},
    {"append", append_proc},
    {"map", map_proc},
    {"filter", filter_proc},
    {"reduce", reduce_proc},
    // 相等性
    {"eq?", eq_proc},
    {"equal?", equal_proc},
};
