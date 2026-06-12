#ifndef ERROR_H
#define ERROR_H

#include <stdexcept>
//词法/语法阶段的错误
class SyntaxError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

//求值阶段的错误
class LispError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

#endif
