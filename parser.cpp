#include "parser.h"
#include"error.h"

Parser::Parser(std::deque<TokenPtr>& tokens) : tokens{tokens} {}

ValuePtr Parser::parse() {
    // 取出队列最前面的 token，并将所有权转移到局部变量 token
    // 必须先 move 再 pop，否则 pop 会直接销毁它
    auto token = std::move(tokens.front());
    tokens.pop_front();

    // 用 getType() 判断 token 的类型
    // 用 static_cast 把基类 Token& 强转成具体子类引用
    // （已经确认了类型，所以这个转型是安全的）
    // 调用子类特有的取值方法，拿到具体的 C++ 值
    // 用这个值构造对应的 Value 子类，包装成 shared_ptr 返回
    if (token->getType() == TokenType::NUMERIC_LITERAL) {
        auto value = static_cast<NumericLiteralToken&>(*token).getValue();
        return std::make_shared<NumericValue>(value);
    }

    if (token->getType() == TokenType::BOOLEAN_LITERAL) {
        auto value = static_cast<BooleanLiteralToken&>(*token).getValue();
        return std::make_shared<BooleanValue>(value);
    }

    if (token->getType() == TokenType::STRING_LITERAL) {
        auto value = static_cast<StringLiteralToken&>(*token).getValue();
        return std::make_shared<StringValue>(value);
    }

    if (token->getType() == TokenType::IDENTIFIER) {
        // getName() 而非 getValue()，因为 IdentifierToken 的接口名不同
        auto name = static_cast<IdentifierToken&>(*token).getName();
        return std::make_shared<SymbolValue>(name);
    }

    //遇到左括号，解析列表
    if (token->getType() == TokenType::LEFT_PAREN) {
        return this->parseTails();
    }

    //处理引号
    auto quoteProcess = [&](const std::string& symbol) -> ValuePtr {
        // 'x 等价于 (quote x)，即 PairValue("quote", PairValue(x, Nil))
        return std::make_shared<PairValue>(
            std::make_shared<SymbolValue>(symbol),
            std::make_shared<PairValue>(
                this->parse(),
                std::make_shared<NilValue>()
            )
        );
    };
    if (token->getType() == TokenType::QUOTE) {
        return quoteProcess("quote");
    }
    if (token->getType() == TokenType::QUASIQUOTE) {
        return quoteProcess("quasiquote");
    }
    if (token->getType() == TokenType::UNQUOTE) {
        return quoteProcess("unquote");
    }
    throw SyntaxError("Unimplemented");
}

ValuePtr Parser::parseTails() {
    // 检查括号是否闭合
    if (tokens.empty()) {
        throw SyntaxError("Unexpected EOF:missing)");
    }

    // 查看队头token的类型
    auto type = tokens.front()->getType();
    // 遇到右括号
    if (type == TokenType::RIGHT_PAREN) {
        tokens.pop_front();
        return std::make_shared<NilValue>();
    }
    // 遇到.
    //  . 在car解析完之后出现，需要写解析car
    auto car = this->parse();
    if (tokens.empty()) {
        throw SyntaxError("Unexpected EOF after element");
    }
    if (tokens.front()->getType() == TokenType::DOT) {
        tokens.pop_front();  // 弹出 .

        auto cdr = this->parse();  // . 后面再解析一个值作为 cdr

        // . 之后解析完 cdr，接下来必须是 )
        if (tokens.empty()) {
            throw SyntaxError("Unexpected EOF: missing ) after dot pair");
        }
        if (tokens.front()->getType() != TokenType::RIGHT_PAREN) {
            throw SyntaxError("Expected ) after dot pair");
        }
        tokens.pop_front();  // 弹出 )

        // 用 car 和 cdr 构造一个对子返回，如 (a . b)
        return std::make_shared<PairValue>(car, cdr);
    }

    // 没有.，继续解析 cdr
    auto cdr = this->parseTails();
    return std::make_shared<PairValue>(car, cdr);
}
