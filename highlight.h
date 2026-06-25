#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

#include <string>
#include <vector>

#include "replxx.hxx"

// 为输入代码的每个字符设置颜色（replxx 回调函数）
void lisp_highlighter(std::string const& context,
                      replxx::Replxx::colors_t& colors);

// Tab 自动补全回调函数（replxx 回调类型）
replxx::Replxx::completions_t lisp_completion(std::string const& context,
                                              int& contextLen);

// 为输出结果添加颜色（ANSI 转义码）
std::string colorize_output(std::string const& text);

#endif
