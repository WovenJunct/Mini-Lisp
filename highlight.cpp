#include "highlight.h"

#include <regex>
#include <unordered_set>

using Replxx = replxx::Replxx;
using Color = Replxx::Color;

// 特殊形式（关键字）
static const std::unordered_set<std::string> SPECIAL_FORMS = {
    "define", "if",  "and",   "or",         "lambda", "cond",
    "begin",  "let", "quote", "quasiquote", "unquote"};

// 内置函数
static const std::unordered_set<std::string> BUILTINS = {
    "car",       "cdr",        "cons",     "list",      "append",
    "map",       "filter",     "reduce",   "length",    "null?",
    "pair?",     "list?",      "symbol?",  "number?",   "boolean?",
    "string?", "integer?"  , "procedure?", "atom?",    "print",     "display",
    "displayln", "newline",    "exit",     "error",     "eval",
    "apply",     "+",          "-",        "*",         "/",  
    "abs",       "expt",       "quotient", "remainder", "modulo",
    "=",         "<",          ">",        "<=",        ">=",
    "even?",     "odd?",       "zero?",    "not",       "eq?",
    "equal?"};

// 所有可补全的关键字
static const std::vector<std::string> COMPLETIONS = {
    // 特殊形式
    "define", "if", "and", "or", "lambda", "cond", "begin",
    "let", "quote", "quasiquote", "unquote",
    // 内置函数
    "car", "cdr", "cons", "list", "append", "map", "filter", "reduce",
    "length", "null?", "pair?", "list?", "symbol?", "number?", "boolean?",
    "string?", "integer?", "procedure?", "atom?",
    "print", "display", "displayln", "newline", "exit", "error", "eval", "apply",
    "+", "-", "*", "/", "abs", "expt", "quotient", "remainder", "modulo",
    "=", "<", ">", "<=", ">=", "even?", "odd?", "zero?", "not", "eq?", "equal?"
};

// 判断字符是否是标识符的一部分
static bool isIdentChar(char c) {
    return isalnum(c) || c == '-' || c == '?' || c == '!' || c == '_';
}

// 高亮回调函数
void lisp_highlighter(std::string const& context, Replxx::colors_t& colors) {
    int len = static_cast<int>(context.length());

    for (int i = 0; i < len; i++) {
        colors[i] = Color::DEFAULT;  // 默认颜色
    }

    int i = 0;
    while (i < len) {
        char c = context[i];

        // 注释：; 开头，灰色直到行尾
        if (c == ';') {
            for (int j = i; j < len; j++) {
                colors[j] = Color::GRAY;
            }
            break;
        }

        // 字符串："..."，绿色
        if (c == '"') {
            colors[i] = Color::BRIGHTGREEN;
            i++;
            while (i < len && context[i] != '"') {
                colors[i] = Color::BRIGHTGREEN;
                if (context[i] == '\\') {
                    i++;
                    if (i < len) colors[i] = Color::BRIGHTGREEN;
                }
                i++;
            }
            if (i < len) {
                colors[i] = Color::BRIGHTGREEN;  // 结尾的 "
                i++;
            }
            continue;
        }

        // 布尔值：#t 或 #f，红色
        if (c == '#' && i + 1 < len &&
            (context[i + 1] == 't' || context[i + 1] == 'f')) {
            colors[i] = Color::BRIGHTRED;
            colors[i + 1] = Color::BRIGHTRED;
            i += 2;
            continue;
        }

        // 数字：黄色
        if (isdigit(c) ||
            (c == '.' && i + 1 < len && isdigit(context[i + 1]))) {
            if (c == '-' || c == '+') {
                // 检查是否是数字的符号
                if (i + 1 < len &&
                    (isdigit(context[i + 1]) || context[i + 1] == '.')) {
                    colors[i] = Color::YELLOW;
                    i++;
                    while (i < len &&
                           (isdigit(context[i]) || context[i] == '.')) {
                        colors[i] = Color::YELLOW;
                        i++;
                    }
                    continue;
                }
            } else {
                colors[i] = Color::YELLOW;
                i++;
                while (i < len && (isdigit(context[i]) || context[i] == '.')) {
                    colors[i] = Color::YELLOW;
                    i++;
                }
                continue;
            }
        }

        // 括号：蓝色
        if (c == '(' || c == ')' || c == '[' || c == ']' || c == '{' ||
            c == '}') {
            colors[i] = Color::BRIGHTBLUE;
            i++;
            continue;
        }

        // 特殊符号：紫色
        if (c == '\'' || c == '`' || c == ',') {
            colors[i] = Color::BRIGHTMAGENTA;
            i++;
            continue;
        }

        // 标识符：检查是否是关键字或内置函数
        if (isalpha(c)) {
            int start = i;
            while (i < len && isIdentChar(context[i])) {
                i++;
            }
            std::string word = context.substr(start, i - start);

            if (SPECIAL_FORMS.count(word)) {
                // 特殊形式：亮紫色
                for (int j = start; j < i; j++) {
                    colors[j] = Color::BRIGHTMAGENTA;
                }
            } else if (BUILTINS.count(word)) {
                // 内置函数：亮青色
                for (int j = start; j < i; j++) {
                    colors[j] = Color::BRIGHTCYAN;
                }
            }
            // 普通标识符保持默认颜色
            continue;
        }

        // 多字符运算符：<= >=
        if ((c == '<' && i + 1 < len && context[i + 1] == '=') ||
            (c == '>' && i + 1 < len && context[i + 1] == '=')) {
            std::string op = context.substr(i, 2);
            if (BUILTINS.count(op)) {
                colors[i] = Color::BRIGHTCYAN;
                colors[i + 1] = Color::BRIGHTCYAN;
            } else {
                colors[i] = Color::BRIGHTBLUE;
                colors[i + 1] = Color::BRIGHTBLUE;
            }
            i += 2;
            continue;
        }

        // 单字符运算符：+ - * / < > = !
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '<' ||
            c == '>' || c == '=' || c == '!') {
            std::string op(1, c);
            if (BUILTINS.count(op)) {
                colors[i] = Color::BRIGHTCYAN;  // 内置函数：青色
            } else {
                colors[i] = Color::BRIGHTBLUE;  // 其他运算符：蓝色
            }
            i++;
            continue;
        }

        i++;
    }
}

// Tab 自动补全回调函数
replxx::Replxx::completions_t lisp_completion(
    std::string const& context,
    int& contextLen) {
    
    replxx::Replxx::completions_t completions;
    
    // 找到光标前的最后一个单词（空格、括号、引号都是分隔符）
    int lastBreak = -1;
    for (int i = static_cast<int>(context.length()) - 1; i >= 0; i--) {
        char c = context[i];
        if (c == ' ' || c == '(' || c == ')' || c == '[' || c == ']' || 
            c == '{' || c == '}' || c == '"' || c == ';' || c == '\'') {
            lastBreak = i;
            break;
        }
    }
    
    std::string prefix = context.substr(lastBreak + 1);
    
    // 设置上下文长度（补全的前缀长度）
    contextLen = static_cast<int>(prefix.length());
    
    // 如果前缀为空，不补全
    if (prefix.empty()) {
        return completions;
    }
    
    // 查找匹配的关键字（前缀匹配）
    for (auto const& keyword : COMPLETIONS) {
        if (keyword.length() >= prefix.length() &&
            keyword.compare(0, prefix.length(), prefix) == 0) {
            completions.emplace_back(keyword.c_str());
        }
    }
    
    return completions;
}

// 为输出添加颜色
std::string colorize_output(std::string const& text) {
    // 空值
    if (text == "()" || text == "nil") {
        return "\033[90m" + text + "\033[0m";  // 灰色
    }

    // 布尔值
    if (text == "#t" || text == "#f") {
        return "\033[1;31m" + text + "\033[0m";  // 亮红色
    }

    // 字符串（带引号）
    if (text.length() >= 2 && text.front() == '"' && text.back() == '"') {
        return "\033[1;32m" + text + "\033[0m";  // 亮绿色
    }

    // 函数
    if (text == "#<procedure>") {
        return "\033[1;36m" + text + "\033[0m";  // 亮青色
    }

    // 数字
    bool isNum = true;
    size_t start = 0;
    if (text[0] == '-' || text[0] == '+') start = 1;
    if (start < text.length()) {
        for (size_t i = start; i < text.length(); i++) {
            if (!isdigit(text[i]) && text[i] != '.') {
                isNum = false;
                break;
            }
        }
    } else {
        isNum = false;
    }

    if (isNum && text.length() > start) {
        return "\033[33m" + text + "\033[0m";  // 黄色
    }

    // 错误信息
    if (text.find("Error") != std::string::npos) {
        return "\033[1;31m" + text + "\033[0m";  // 亮红色
    }

    return text;
}
