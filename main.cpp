#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "./tokenizer.h"
#include"./value.h"
#include"parser.h"
#include"eval_env.h"
#include "./error.h"
//主程序代码
int main(int argc, char* argv[]) {
    auto env = EvalEnv::createGlobal();

    if (argc == 2) {
        // 文件模式
        std::ifstream file(argv[1]);
        if (!file) {
            std::cerr << "Error: cannot open file " << argv[1] << std::endl;
            return 1;
        }
        std::stringstream ss;
        ss << file.rdbuf();
        std::string content = ss.str();
        try {
            auto tokens = Tokenizer::tokenize(content);
            // 循环解析所有顶层表达式
            while (!tokens.empty()) {
                Parser parser(tokens);
                auto value = parser.parse();
                env->eval(value);  // 文件模式不输出求值结果
            }
        } catch (std::runtime_error& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
        return 0;

    } else {
        // REPL 模式
        while (true) {
            try {
                std::cout << ">>> ";
                std::string input;
                int depth = 0;
                bool inString = false;  // 是否在字符串内，避免把 "(" 计入深度

                while (true) {
                    std::string line;
                    std::getline(std::cin, line);
                    if (std::cin.eof()) std::exit(0);

                    // 扫描这一行，统计括号深度
                    for (int i = 0; i < (int)line.size(); i++) {
                        char c = line[i];
                        if (inString) {
                            if (c == '\\')
                                i++;  // 跳过转义字符
                            else if (c == '"')
                                inString = false;
                        } else {
                            if (c == '"')
                                inString = true;
                            else if (c == ';')
                                break;  // 注释，忽略后续
                            else if (c == '(')
                                depth++;
                            else if (c == ')')
                                depth--;
                        }
                    }

                    input += line + " ";  // 拼接到总输入，行间用空格分隔

                    // 括号已匹配且输入不为空，表达式完整
                    if (depth <= 0 && !input.empty()) break;

                    // 还没匹配完，显示续行提示符
                    std::cout << "... ";
                }

                if (input.empty() || input == " ") continue;

                auto tokens = Tokenizer::tokenize(input);
                if (tokens.empty()) continue;

                Parser parser(tokens);
                auto value = parser.parse();
                auto result = env->eval(value);
                std::cout << result->toString() << std::endl;
            } catch (std::runtime_error& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
    }
}


//测试代码
//#include "rjsj_test.hpp"
//struct TestCtx {
//    std::shared_ptr<EvalEnv> env = EvalEnv::createGlobal();
//    std::string eval(std::string input) {
//        auto tokens = Tokenizer::tokenize(input);
//        Parser parser(tokens);
//        auto value = parser.parse();
//        auto result = env->eval(std::move(value));
//        return result->toString();
//    }
//};
//int main() {
//    RJSJ_TEST(TestCtx, Lv2, Lv3, Lv4, Lv5, Lv5Extra, Lv6, Lv7, Lv7Lib, Sicp);
//}
