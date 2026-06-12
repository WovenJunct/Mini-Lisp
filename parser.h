#ifndef PARSER_H
#define PARSER_H

#include<memory>
#include<deque>
#include"token.h"
#include"value.h"

class Parser {
    // 存储待解析的 token 序列
    std::deque<TokenPtr>& tokens;  //引用，不持有所有权
    ValuePtr parseTails();

public:
    // 接受 token 序列
    Parser(std::deque<TokenPtr> &tokens);   //接受引用
    ValuePtr parse();
};

#endif
