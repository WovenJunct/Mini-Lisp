#ifndef COMMANDS_H
#define COMMANDS_H

#include <string>

// 显示启动信息
void show_welcome();

// 检查是否是内置命令（help, clear, quit）
// 如果是，执行命令并返回 true；否则返回 false
bool handle_command(const std::string& input);

// 漂亮打印输出（嵌套列表自动换行缩进）
std::string pretty_print(const std::string& raw_output);

#endif
