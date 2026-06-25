#include "commands.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>

// 显示启动信息
void show_welcome() {
    std::cout << "\033[1;36mMini-Lisp Interpreter v1.0\033[0m" << std::endl;
    std::cout << "Type \033[1;33m(help)\033[0m for help, \033[1;33m(clear)\033[0m to clear screen, \033[1;33m(quit)\033[0m to exit." << std::endl;
    std::cout << std::endl;
}

// 显示帮助信息
static void show_help() {
    std::cout << "\033[1;36m=== Help ===\033[0m" << std::endl;
    std::cout << "\033[1;33m[Special Forms]\033[0m" << std::endl;
    std::cout << "  define, if, and, or, lambda, cond, begin, let" << std::endl;
    std::cout << "  quote, quasiquote, unquote" << std::endl;
    std::cout << "\033[1;33m[Built-in Functions]\033[0m" << std::endl;
    std::cout << "  car, cdr, cons, list, append, map, filter, reduce" << std::endl;
    std::cout << "  length, null?, pair?, list?, symbol?, number?" << std::endl;
    std::cout << "  boolean?, string?, integer?, procedure?, atom?" << std::endl;
    std::cout << "  print, display, displayln, newline, exit, error, eval, apply" << std::endl;
    std::cout << "  +, -, *, /, abs, expt, quotient, remainder, modulo" << std::endl;
    std::cout << "  =, <, >, <=, >=, even?, odd?, zero?, not, eq?, equal?" << std::endl;
    std::cout << "\033[1;33m[Commands]\033[0m" << std::endl;
    std::cout << "  \033[1;32m(help)\033[0m  - show this help" << std::endl;
    std::cout << "  \033[1;32m(clear)\033[0m - clear screen" << std::endl;
    std::cout << "  \033[1;32m(quit)\033[0m  - exit interpreter" << std::endl;
}

// 清屏
static void clear_screen() {
    std::cout << "\033[2J\033[1;1H" << std::flush;
}

// 检查是否是内置命令并执行
bool handle_command(const std::string& input) {
    // 去除前后空格
    std::string trimmed = input;
    size_t start = trimmed.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return false;
    trimmed = trimmed.substr(start);
    size_t end = trimmed.find_last_not_of(" \t\n\r");
    if (end != std::string::npos) trimmed = trimmed.substr(0, end + 1);
    
    // 检查命令
    if (trimmed == "help" || trimmed == "(help)") {
        show_help();
        return true;
    }
    if (trimmed == "clear" || trimmed == "(clear)") {
        clear_screen();
        return true;
    }
    if (trimmed == "quit" || trimmed == "(quit)" || trimmed == "exit" || trimmed == "(exit)") {
        std::exit(0);
    }
    
    return false;
}

// 漂亮打印输出（只在顶层换行，嵌套列表保持单行）
std::string pretty_print(const std::string& raw_output) {
    // 如果输出太短（少于 40 个字符），直接返回
    if (raw_output.length() < 40) {
        return raw_output;
    }
    
    // 检查是否是列表（以 '(' 开头，以 ')' 结尾）
    if (raw_output.front() != '(' || raw_output.back() != ')') {
        return raw_output;
    }
    
    // 提取顶层元素（跳过外层括号）
    std::string inner = raw_output.substr(1, raw_output.length() - 2);
    
    // 分割顶层元素
    std::vector<std::string> elements;
    std::string current = "";
    int depth = 0;
    bool in_string = false;
    
    for (size_t i = 0; i < inner.length(); i++) {
        char c = inner[i];
        
        // 处理字符串
        if (c == '"' && (i == 0 || inner[i - 1] != '\\')) {
            in_string = !in_string;
        }
        
        if (!in_string) {
            if (c == '(') {
                depth++;
                current += c;
            } else if (c == ')') {
                depth--;
                current += c;
            } else if (c == ' ' && depth == 0) {
                // 顶层空格，分割元素
                if (!current.empty()) {
                    elements.push_back(current);
                    current = "";
                }
                // 跳过多余空格
                while (i + 1 < inner.length() && inner[i + 1] == ' ') {
                    i++;
                }
            } else {
                current += c;
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        elements.push_back(current);
    }
    
    // 如果元素少于 3 个，不需要换行
    if (elements.size() < 3) {
        return raw_output;
    }
    
    // 格式化输出：每个元素一行
    std::string result = "(\n";
    for (size_t i = 0; i < elements.size(); i++) {
        result += "  " + elements[i];
        if (i + 1 < elements.size()) {
            result += "\n";
        }
    }
    result += "\n)";  // 最后一个括号单独一行
    
    return result;
}
