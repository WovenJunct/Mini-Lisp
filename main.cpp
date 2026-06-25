#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "./tokenizer.h"
#include"./value.h"
#include"parser.h"
#include"eval_env.h"
#include "./error.h"
#include "builtins.h"
// 代码高亮相关
#include "highlight.h"
// 内置命令（help, clear, pretty_print）
#include "commands.h"

// Windows 下设置控制台为 UTF-8 编码
#ifdef _WIN32
#include <windows.h>
#endif

//主程序代码
int main(int argc, char* argv[]) {
    // 设置控制台编码为 UTF-8（解决中文乱码）
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif
    
    auto env = EvalEnv::createGlobal();

    if (argc == 2) {
        // 文件模式
        g_replMode = false;
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
        // REPL 模式（使用 replxx）
        g_replMode = true;
        replxx::Replxx rx;
        rx.install_window_change_handler();

        // 加载历史记录
        std::string historyFile = ".mini-lisp_history";
        {
            std::ifstream hf(historyFile);
            if (hf.good()) rx.history_load(hf);
        }
        rx.set_max_history_size(128);

        // 设置高亮回调
        rx.set_highlighter_callback(lisp_highlighter);
        
        // 设置自动补全回调
        rx.set_completion_callback(lisp_completion);
        
        // 配置补全选项
        rx.set_word_break_characters(" \t\n();");
        rx.set_completion_count_cutoff(128);
        rx.set_double_tab_completion(false);
        rx.set_complete_on_empty(true);
        rx.set_beep_on_ambiguous_completion(false);

        // 显示启动信息
        show_welcome();

        std::string prompt = "\033[1;32m>>>\033[0m ";

        while (true) {
            char const* cinput = rx.input(prompt);
            if (cinput == nullptr) break;

            std::string input(cinput);
            if (input.empty()) continue;

            rx.history_add(input);

            // 多行输入支持（括号匹配）
            int depth = 0;
            bool inString = false;
            std::string fullInput = input;

            // 扫描当前输入的括号深度
            for (size_t i = 0; i < input.size(); i++) {
                char c = input[i];
                if (inString) {
                    if (c == '\\')
                        i++;
                    else if (c == '"')
                        inString = false;
                } else {
                    if (c == '"')
                        inString = true;
                    else if (c == ';')
                        break;
                    else if (c == '(')
                        depth++;
                    else if (c == ')')
                        depth--;
                }
            }

            // 继续输入直到括号匹配
            while (depth > 0) {
                // 计算自动缩进：找到最后一个未匹配的 '('，缩进到其后的位置
                std::string indent = "";
                {
                    // 找到 fullInput 中最后一个未匹配的 '('
                    int tempDepth = 0;
                    int lastOpenPos = -1;
                    for (int i = static_cast<int>(fullInput.length()) - 1; i >= 0; i--) {
                        char ch = fullInput[i];
                        if (ch == ')') tempDepth++;
                        else if (ch == '(') {
                            if (tempDepth == 0) {
                                lastOpenPos = i;
                                break;
                            }
                            tempDepth--;
                        }
                    }
                    
                    if (lastOpenPos >= 0) {
                        // 计算缩进： '(' 后面的内容作为缩进基础
                        int spaces = lastOpenPos + 1;
                        // 如果 '(' 后面有内容，用那个位置的偏移
                        if (lastOpenPos + 1 < static_cast<int>(fullInput.length())) {
                            // 从 '(' 后面到行尾计算列偏移
                            int col = 0;
                            for (int i = lastOpenPos + 1; i < static_cast<int>(fullInput.length()); i++) {
                                if (fullInput[i] == '\n') {
                                    col = 0;
                                } else {
                                    col++;
                                }
                            }
                            indent = std::string(col, ' ');
                        } else {
                            // '(' 后面没有内容，缩进 2 个空格
                            indent = "  ";
                        }
                    } else {
                        // 没有找到未匹配的 '('，缩进 2 个空格
                        indent = "  ";
                    }
                }
                
                // 预加载缩进空格
                rx.set_preload_buffer(indent);
                
                std::string continuationPrompt = "\033[1;33m...\033[0m ";
                cinput = rx.input(continuationPrompt);
                if (cinput == nullptr) break;

                std::string line(cinput);
                fullInput += " " + line;

                for (size_t i = 0; i < line.size(); i++) {
                    char c = line[i];
                    if (inString) {
                        if (c == '\\')
                            i++;
                        else if (c == '"')
                            inString = false;
                    } else {
                        if (c == '"')
                            inString = true;
                        else if (c == ';')
                            break;
                        else if (c == '(')
                            depth++;
                        else if (c == ')')
                            depth--;
                    }
                }
            }

            if (fullInput.empty() || fullInput == " ") continue;

            // 检查是否是内置命令
            if (handle_command(fullInput)) {
                continue;
            }

            try {
                auto tokens = Tokenizer::tokenize(fullInput);
                if (tokens.empty()) continue;

                Parser parser(tokens);
                auto value = parser.parse();
                auto result = env->eval(value);
                std::string output = result->toString();
                // 漂亮打印 + 输出高亮
                std::cout << colorize_output(pretty_print(output)) << std::endl;
            } catch (std::runtime_error& e) {
                std::cerr << "\033[1;31mError: " << e.what() << "\033[0m"
                          << std::endl;
            }
        }

        // 保存历史记录
        {
            std::ofstream hf(historyFile);
            rx.history_save(hf);
        }
    }
}
