#include "./eval_env.h"

#include <algorithm>
#include <iterator>

#include "./builtins.h"
#include "./error.h"
#include "./forms.h"

using namespace std::literals;

std::shared_ptr<EvalEnv> EvalEnv::createGlobal() {
    auto env = std::shared_ptr<EvalEnv>(new EvalEnv());
    for (auto& [name, func] : BUILTINS) {
        env->symbolTable[name] = std::make_shared<BuiltinProcValue>(func);
    }
    return env;
}

std::shared_ptr<EvalEnv> EvalEnv::createChild() {
    return std::shared_ptr<EvalEnv>(new EvalEnv(shared_from_this()));
}

void EvalEnv::defineVar(const std::string& name, ValuePtr value) {
    symbolTable[name] = value;
}

ValuePtr EvalEnv::lookupVar(const std::string& name) {
    auto it = symbolTable.find(name);
    if (it != symbolTable.end()) return it->second;
    if (parent) return parent->lookupVar(name);
    throw LispError("Variable " + name + " not defined.");
}

std::vector<ValuePtr> EvalEnv::evalList(ValuePtr expr) {
    std::vector<ValuePtr> result;
    for (auto& v : expr->toVector()) {
        result.push_back(this->eval(v));
    }
    return result;
}

ValuePtr EvalEnv::apply(ValuePtr proc, std::vector<ValuePtr> args) {
    if (typeid(*proc) == typeid(BuiltinProcValue)) {
        return static_cast<BuiltinProcValue&>(*proc).call(args, *this);
    } else if (typeid(*proc) == typeid(LambdaValue)) {
        return static_cast<LambdaValue&>(*proc).apply(args);
    } else {
        throw LispError("Not a procedure.");
    }
}

ValuePtr EvalEnv::eval(ValuePtr expr) {
    if (expr->isSelfEvaluating()) return expr;

    if (expr->isNil()) throw LispError("Evaluating nil is prohibited.");

    if (auto name = expr->asSymbol()) {
        return lookupVar(*name);
    }

    if (typeid(*expr) == typeid(PairValue)) {
        auto& pair = static_cast<PairValue&>(*expr);

        if (auto name = pair.getCar()->asSymbol()) {
            auto it = SPECIAL_FORMS.find(*name);
            if (it != SPECIAL_FORMS.end()) {
                auto args = pair.getCdr()->toVector();
                return it->second(args, *this);
            }
        }

        auto v = expr->toVector();
        ValuePtr proc = eval(v[0]);
        std::vector<ValuePtr> args;
        for (size_t i = 1; i < v.size(); i++) {
            args.push_back(eval(v[i]));
        }
        return apply(proc, args);
    }

    throw LispError("Unknown expression type.");
}
