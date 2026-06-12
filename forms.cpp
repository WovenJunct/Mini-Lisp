#include "./forms.h"

#include "./error.h"
#include "./eval_env.h"
#include <functional>
using namespace std::literals;

// 辅助函数：判断一个值是否为 #f
static bool isFalse(ValuePtr v) {
    return typeid(*v) == typeid(BooleanValue) && !v->asBoolean();
}

// define：支持 (define x val) 和 (define (f params...) body...)
ValuePtr defineForm(const std::vector<ValuePtr>& args, EvalEnv& env) {
    if (args.size() < 2) throw LispError("Malformed define.");

    if (auto name = args[0]->asSymbol()) {
        // 普通变量定义：(define x val)
        ValuePtr val = env.eval(args[1]);
        env.defineVar(*name, val);
        return std::make_shared<NilValue>();

    } else if (typeid(*args[0]) == typeid(PairValue)) {
        // 函数简写：(define (f x y) body...)
        // 等价于 (define f (lambda (x y) body...))
        auto nameAndParams = args[0]->toVector();

        auto fname = nameAndParams[0]->asSymbol();
        if (!fname)
            throw LispError(
                "Malformed define: function name must be a symbol.");

        // 收集形参名
        std::vector<std::string> params;
        for (size_t i = 1; i < nameAndParams.size(); i++) {
            auto p = nameAndParams[i]->asSymbol();
            if (!p)
                throw LispError(
                    "Malformed define: parameter must be a symbol.");
            params.push_back(*p);
        }

        // 过程体是 args[1] 以后的所有元素
        std::vector<ValuePtr> body(args.begin() + 1, args.end());
        auto lambda =
            std::make_shared<LambdaValue>(params, body, env.shared_from_this());
        env.defineVar(*fname, lambda);
        return std::make_shared<NilValue>();

    } else {
        throw LispError("Malformed define.");
    }
}

// quote：不求值，直接返回参数本身
ValuePtr quoteForm(const std::vector<ValuePtr>& args, EvalEnv& env) {
    if (args.size() != 1) throw LispError("quote requires exactly 1 argument.");
    return args[0];
}

// if：(if cond true-branch false-branch)
// 只有 #f 是假，其他值（包括空表）都是真
ValuePtr ifForm(const std::vector<ValuePtr>& args, EvalEnv& env) {
    if (args.size() < 2 || args.size() > 3) throw LispError("Malformed if.");

    ValuePtr cond = env.eval(args[0]);
    if (!isFalse(cond)) {
        return env.eval(args[1]);  // 条件为真，求值真分支
    } else if (args.size() == 3) {
        return env.eval(args[2]);  // 条件为假，求值假分支
    } else {
        return std::make_shared<NilValue>();  // 没有假分支，返回空表
    }
}

// and：从左到右求值，遇到 #f 立即返回 #f，否则返回最后一个值
// (and) 返回 #t
ValuePtr andForm(const std::vector<ValuePtr>& args, EvalEnv& env) {
    if (args.empty()) return std::make_shared<BooleanValue>(true);
    ValuePtr result;
    for (auto& arg : args) {
        result = env.eval(arg);
        if (isFalse(result))
            return std::make_shared<BooleanValue>(false);  // 短路
    }
    return result;  // 所有值均为真，返回最后一个
}

// or：从左到右求值，遇到非 #f 的值立即返回它
// (or) 返回 #f
ValuePtr orForm(const std::vector<ValuePtr>& args, EvalEnv& env) {
    for (auto& arg : args) {
        ValuePtr result = env.eval(arg);
        if (!isFalse(result)) return result;  // 短路
    }
    return std::make_shared<BooleanValue>(false);
}

// lambda：(lambda (params...) body...)
// 将参数列表和过程体打包为 LambdaValue，捕获当前环境为闭包
ValuePtr lambdaForm(const std::vector<ValuePtr>& args, EvalEnv& env) {
    if (args.size() < 2) throw LispError("Malformed lambda.");

    // 收集形参名
    std::vector<std::string> params;
    if (!args[0]->isNil()) {  // 参数列表可以为空 ()
        auto paramList = args[0]->toVector();
        for (auto& p : paramList) {
            auto name = p->asSymbol();
            if (!name) throw LispError("Lambda parameter must be a symbol.");
            params.push_back(*name);
        }
    }

    // 过程体是 args[1] 以后的所有元素
    std::vector<ValuePtr> body(args.begin() + 1, args.end());
    return std::make_shared<LambdaValue>(params, body, env.shared_from_this());
}

// cond
ValuePtr condForm(const std::vector<ValuePtr>& args, EvalEnv& env) {
    for (auto& clause : args) {
        auto clauseVec = clause->toVector();
        if (clauseVec.empty()) throw LispError("Malformed cond clause.");
        if (auto sym = clauseVec[0]->asSymbol(); sym && *sym == "else") {
            ValuePtr result = std::make_shared<NilValue>();
            for (size_t i = 1; i < clauseVec.size(); i++)
                result = env.eval(clauseVec[i]);
            return result;
        }
        ValuePtr cond = env.eval(clauseVec[0]);
        if (!isFalse(cond)) {
            if (clauseVec.size() == 1) return cond;
            ValuePtr result;
            for (size_t i = 1; i < clauseVec.size(); i++)
                result = env.eval(clauseVec[i]);
            return result;
        }
    }
    return std::make_shared<NilValue>();
}

// begin
ValuePtr beginForm(const std::vector<ValuePtr>& args, EvalEnv& env) {
    if (args.empty()) throw LispError("begin requires at least 1 expression.");
    ValuePtr result;
    for (auto& expr : args) result = env.eval(expr);
    return result;
}

// let
ValuePtr letForm(const std::vector<ValuePtr>& args, EvalEnv& env) {
    if (args.size() < 2) throw LispError("Malformed let.");
    auto bindings = args[0]->toVector();
    std::vector<std::string> params;
    std::vector<ValuePtr> vals;
    for (auto& binding : bindings) {
        auto bindVec = binding->toVector();
        if (bindVec.size() != 2) throw LispError("Malformed let binding.");
        auto name = bindVec[0]->asSymbol();
        if (!name) throw LispError("let binding name must be a symbol.");
        params.push_back(*name);
        vals.push_back(env.eval(bindVec[1]));
    }
    auto child = env.createChild();
    for (size_t i = 0; i < params.size(); i++)
        child->defineVar(params[i], vals[i]);
    ValuePtr result = std::make_shared<NilValue>();
    for (size_t i = 1; i < args.size(); i++) result = child->eval(args[i]);
    return result;
}

// quasiquote
ValuePtr quasiquoteForm(const std::vector<ValuePtr>& args, EvalEnv& env) {
    if (args.size() != 1)
        throw LispError("quasiquote requires exactly 1 argument.");
    std::function<ValuePtr(ValuePtr)> process = [&](ValuePtr tmpl) -> ValuePtr {
        if (typeid(*tmpl) == typeid(PairValue)) {
            auto& pair = static_cast<PairValue&>(*tmpl);
            if (auto sym = pair.getCar()->asSymbol();
                sym && *sym == "unquote") {
                auto inner = pair.getCdr()->toVector();
                if (inner.size() != 1) throw LispError("Malformed unquote.");
                return env.eval(inner[0]);
            }
            return std::make_shared<PairValue>(process(pair.getCar()),
                                               process(pair.getCdr()));
        }
        return tmpl;
    };
    return process(args[0]);
}

// unquote（在 quasiquote 外使用报错）
ValuePtr unquoteForm(const std::vector<ValuePtr>& args, EvalEnv& env) {
    throw LispError("unquote used outside of quasiquote.");
}

// 更新 SPECIAL_FORMS
const std::unordered_map<std::string, SpecialFormType*> SPECIAL_FORMS = {
    {"define", defineForm},   {"quote", quoteForm},
    {"if", ifForm},           {"and", andForm},
    {"or", orForm},           {"lambda", lambdaForm},
    {"cond", condForm},       {"begin", beginForm},
    {"let", letForm},         {"quasiquote", quasiquoteForm},
    {"unquote", unquoteForm},
};
