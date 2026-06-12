# Mini-Lisp 解释器

一个用 C++ 实现的 Mini-Lisp 解释器，支持 REPL 交互模式和文件执行模式，实现了 Mini-Lisp 语言规范定义的所有特性。


## 功能特性

- **完整的词法分析与语法分析**：支持所有 Lisp 数据类型的字面量
- **词法作用域与闭包**：正确实现了环境链与 Lambda 闭包
- **47 个内置过程**：覆盖算术、比较、列表操作、类型检查等
- **完整的特殊形式**：`define` `lambda` `if` `cond` `let` `begin` `and` `or` `quote` `quasiquote` 等
- **REPL 模式**：交互式读取-求值-输出循环
- **文件模式**：从命令行读取 `.scm` 源文件并执行

---

## 项目结构

```
mini-lisp/
├── error.h          # 自定义异常类
├── token.h          # Token 类型定义
├── token.cpp        # Token 类型实现
├── tokenizer.h      # 词法分析器定义
├── tokenizer.cpp    # 词法分析器实现
├── value.h          # Lisp 值类型定义
├── value.cpp        # Lisp 值类型实现
├── parser.h         # 语法分析器定义
├── parser.cpp       # 语法分析器实现
├── eval_env.h       # 求值环境定义
├── eval_env.cpp     # 求值环境实现
├── builtins.h       # 内置过程声明
├── builtins.cpp     # 内置过程实现
├── forms.h          # 特殊形式声明
├── forms.cpp        # 特殊形式实现
├── main.cpp         # 程序入口
└── lv7-answer.scm   # 快速排序示例程序
```

---

## 编译与运行

### 编译

使用 Visual Studio 直接打开项目编译，或使用支持 C++20 的编译器：

```bash
g++ -std=c++20 -o mini-lisp main.cpp token.cpp tokenizer.cpp value.cpp parser.cpp eval_env.cpp builtins.cpp forms.cpp
```

### REPL 模式

直接运行可执行文件，进入交互式界面：

```
$ ./mini-lisp
>>> (+ 1 2)
3
>>> (define (square x) (* x x))
()
>>> (square 5)
25
>>> (quit)
```

### 文件模式

将源文件路径作为命令行参数传入：

```bash
./mini-lisp lv7-answer.scm
```

---

## 实现详解

### 第一阶段：值类型体系（`value.h` / `value.cpp`）

Mini-Lisp 中所有数据的 C++ 内存表示。所有值都继承自抽象基类 `Value`，通过 `std::shared_ptr<Value>`（即 `ValuePtr`）传递，使用共享所有权语义。

**类型体系**：

| 类型 | C++ 类 | 外部表示示例 |
|---|---|---|
| 布尔 | `BooleanValue` | `#t` / `#f` |
| 数值 | `NumericValue` | `42`、`3.14` |
| 字符串 | `StringValue` | `"hello"` |
| 空表 | `NilValue` | `()` |
| 符号 | `SymbolValue` | `define`、`eq?` |
| 对子 | `PairValue` | `(1 . 2)`、`(1 2 3)` |
| 内置过程 | `BuiltinProcValue` | `#<procedure>` |
| Lambda 过程 | `LambdaValue` | `#<procedure>` |

**`PairValue` 的列表输出**：采用 `toString()` + `toStringTail()` 两函数协作的方式，递归输出嵌套列表，避免多余的括号。`toStringTail()` 根据 `cdr` 的类型分三种情况处理：末尾是 `NilValue` 则加 `)`，是 `PairValue` 则递归，否则输出点对形式。

**工具方法**：`Value` 基类上定义了若干辅助方法供求值器使用：

- `isNil()` / `isSelfEvaluating()`：类型快速判断
- `asSymbol()`：返回 `std::optional<std::string>`，供特殊形式识别符号
- `toVector()`：将列表转换为 `vector<ValuePtr>`，供求值器访问列表元素
- `isNumber()` / `asNumber()`：数值类型访问
- `asBoolean()`：布尔值访问

---

### 第二阶段：词法分析（`token.h` / `token.cpp` / `tokenizer.h` / `tokenizer.cpp`）

将输入字符串切分为有意义的 Token 序列。

**Token 类型**：

| Token | 对应语法 |
|---|---|
| `LEFT_PAREN` / `RIGHT_PAREN` | `(` `)` |
| `QUOTE` / `QUASIQUOTE` / `UNQUOTE` | `'` `` ` `` `,` |
| `DOT` | `.` |
| `BOOLEAN_LITERAL` | `#t` / `#f` |
| `NUMERIC_LITERAL` | 数字字面量 |
| `STRING_LITERAL` | 字符串字面量 |
| `IDENTIFIER` | 标识符 |

**词法分析器设计**：`Tokenizer` 类采用工厂方法模式，构造函数私有，只能通过静态方法 `Tokenizer::tokenize(string)` 创建并获取结果，返回 `std::deque<TokenPtr>`（`unique_ptr` 管理所有权）。

`nextToken()` 方法通过一个 `pos` 指针逐字符扫描，按以下顺序处理：
1. `;` 开头 → 跳过注释行
2. 空白字符 → 跳过
3. `( ) ' ` , ` → 单字符 Token
4. `#` → 读取后续字符，解析为布尔字面量
5. `"` → 进入字符串扫描子循环，支持 `\n`、`\"` 等转义序列
6. 其他 → 扫描到边界，尝试解析为数字（`std::stod`），失败则作为标识符

**数字与标识符的区分**：先尝试 `std::stod` 转换，失败才视为标识符，这样 `+`、`-` 单独出现时正确识别为标识符（即函数名）。

---

### 第三阶段：语法分析（`parser.h` / `parser.cpp`）

将 Token 序列解析为 Value 树（AST）。

**接口设计**：`Parser` 接受 `std::deque<TokenPtr>&` 引用，每次 `parse()` 调用消费若干 Token 并返回一个完整的 `ValuePtr`，文件模式下可循环调用直到队列为空。

**`parse()` 的处理逻辑**：

- 字面量 Token（数值、布尔、字符串、标识符）→ 直接转换为对应 Value
- `LEFT_PAREN` → 弹出后调用 `parseTails()` 处理括号内容
- `QUOTE` / `QUASIQUOTE` / `UNQUOTE` → 展开为等价的列表形式，例如 `'x` 展开为 `(quote x)`

**`parseTails()` 的递归下降**：处理 `(` 之后的内容，三种情形：
1. 队头是 `)` → 返回 `NilValue`，列表结束
2. 解析一个 `car`，队头是 `.` → 再解析一个 `cdr`，期望 `)`，返回 `PairValue(car, cdr)`
3. 解析一个 `car`，递归调用自身得到 `cdr`，返回 `PairValue(car, cdr)`

`parse()` 和 `parseTails()` 互相递归，能正确处理任意深度的嵌套结构。

---

### 第四阶段：求值环境（`eval_env.h` / `eval_env.cpp`）

**环境链设计**：`EvalEnv` 继承自 `std::enable_shared_from_this<EvalEnv>`，每个环境持有：
- `symbolTable`：当前层的变量绑定（`unordered_map<string, ValuePtr>`）
- `parent`：父环境的 `shared_ptr`，形成环境链

变量查找 `lookupVar()` 先在当前层查找，找不到则递归查找父环境，直到没有父环境则抛出 `LispError`。

**工厂方法**：构造函数设为私有，通过两个静态/成员工厂方法创建：
- `EvalEnv::createGlobal()`：创建含所有内置过程的全局环境
- `env->createChild()`：以当前环境为父环境创建子环境（用于 Lambda 调用）

这保证了所有 `EvalEnv` 对象都被 `shared_ptr` 管理，`shared_from_this()` 能正确工作。

**`eval()` 的求值流程**：

```
1. 自求值表达式（数值、布尔、字符串、过程）→ 直接返回
2. 空表 → 抛出 LispError
3. 符号 → 调用 lookupVar() 查找环境链
4. 列表（PairValue）：
   a. 检查首元素是否在 SPECIAL_FORMS 中 → 调用对应特殊形式处理函数
   b. 否则为过程调用：对所有元素求值，第一个为过程，其余为参数 → apply()
```

**`apply()` 的调用分发**：根据过程类型分发：
- `BuiltinProcValue` → 调用内部函数指针 `func(args, env)`
- `LambdaValue` → 调用 `LambdaValue::apply(args)`

---

### 第五阶段：内置过程（`builtins.h` / `builtins.cpp`）

所有内置过程统一签名：

```cpp
using BuiltinFuncType = ValuePtr(const std::vector<ValuePtr>&, EvalEnv&);
```

通过一个 `unordered_map` 集中注册，`EvalEnv` 构造时遍历注册表将所有内置过程加入全局符号表。

**已实现的 47 个内置过程**：

| 类别 | 过程 |
|---|---|
| 核心 IO | `print` `display` `displayln` `newline` `exit` `error` `eval` `apply` |
| 算术 | `+` `-` `*` `/` `abs` `expt` `quotient` `remainder` `modulo` |
| 比较 | `=` `<` `>` `<=` `>=` `even?` `odd?` `zero?` `not` |
| 类型检查 | `boolean?` `number?` `integer?` `string?` `symbol?` `null?` `pair?` `list?` `procedure?` `atom?` |
| 列表操作 | `car` `cdr` `cons` `length` `list` `append` `map` `filter` `reduce` |
| 相等性 | `eq?` `equal?` |

**`eq?` 与 `equal?` 的区别**：
- `eq?`：恒等性比较。对字符串和对子比较指针地址；对其他类型（数值、符号、布尔、空表）比较值。
- `equal?`：相等性比较。递归比较对子的 `car` 和 `cdr`；其他类型比较外部表示字符串。

**`atom?` 的正确实现**：根据规范，`atom?` 仅对布尔、数值、字符串、符号、空表返回 `#t`，过程类型和对子类型返回 `#f`（不能用"非 Pair 即 atom"这一错误逻辑）。

---

### 第六阶段：特殊形式（`forms.h` / `forms.cpp`）

所有特殊形式统一签名：

```cpp
using SpecialFormType = ValuePtr(const std::vector<ValuePtr>&, EvalEnv&);
```

接收的参数是去掉形式名之后的 CDR 部分，通过 `SPECIAL_FORMS` map 注册，求值器在检测到首元素为已知特殊形式符号时调用。

**已实现的特殊形式**：

| 形式 | 说明 |
|---|---|
| `define` | 变量定义，支持 `(define x val)` 和 `(define (f params) body)` 两种写法 |
| `lambda` | 创建 Lambda 过程，捕获当前环境为闭包 |
| `quote` | 不求值，直接返回表达式本身 |
| `if` | 条件求值，只有 `#f` 是假值 |
| `and` | 从左到右短路求值，遇 `#f` 返回 `#f`，否则返回最后一个值 |
| `or` | 从左到右短路求值，遇非 `#f` 值返回它，否则返回 `#f` |
| `cond` | 多条件判断，支持 `else` 子句 |
| `begin` | 顺序求值，返回最后一个表达式的值 |
| `let` | 局部绑定，创建子环境并在其中求值过程体 |
| `quasiquote` | 部分字面表达式，遇到 `unquote` 时对内部表达式求值 |
| `unquote` | 仅在 `quasiquote` 内有效 |

---

### 第七阶段：Lambda 与闭包（`value.cpp` 中的 `LambdaValue`）

`LambdaValue` 存储三个关键信息：

```cpp
std::vector<std::string> params;   // 形参名列表
std::vector<ValuePtr> body;        // 过程体（多条语句）
std::shared_ptr<EvalEnv> closure;  // 定义时的环境（闭包）
```

**调用时的环境处理**：`LambdaValue::apply()` 的执行步骤：
1. 检查实参数量与形参数量是否匹配
2. 以 `closure`（定义时环境）为父环境，调用 `closure->createChild()` 创建新环境
3. 将形参逐一绑定到实参：`env->defineVar(params[i], args[i])`
4. 在新环境中依次求值 `body` 的每条语句，返回最后一条的结果

这实现了 Mini-Lisp 规范定义的**词法作用域**：Lambda 调用时的上级环境是其**定义时**的环境，而非调用时的环境，从而支持闭包：

```scheme
(define (compose f g) (lambda (x) (f (g x))))
(define add2 (compose add1 add1))
(add2 42)  ; => 44，能正确从闭包中找到 f 和 g
```

---

### 第八阶段：文件模式（`main.cpp`）

`main()` 通过 `argc` 判断运行模式：
- **无参数**：进入 REPL 循环，逐行读取、解析、求值、输出
- **有文件路径参数**：读取整个文件内容，循环解析所有顶层表达式并求值，不输出求值结果（只有 `print`/`display` 等 IO 过程才产生输出）

文件模式的多表达式解析通过 `Parser` 接受引用实现：每次 `parse()` 消费 deque 中已解析的 Token，循环直到 deque 为空。

---

## 示例：快速排序（`lv7-answer.scm`）

```scheme
(define (quicksort lst)
  (if (null? lst)
      '()
      (let ((pivot (car lst))
            (rest (cdr lst)))
        (append
          (quicksort (filter (lambda (x) (< x pivot)) rest))
          (list pivot)
          (quicksort (filter (lambda (x) (>= x pivot)) rest))))))

(define data '(12 71 2 15 29 82 87 8 18 66 81 25 63 97 40 3 93 58 53 31 47))
(display (quicksort data))
(newline)
```

运行结果：

```
(2 3 8 12 15 18 25 29 31 40 47 53 58 63 66 71 81 82 87 93 97)
```

---

## 错误处理

项目定义了两类异常，均继承自 `std::runtime_error`：

- `SyntaxError`：词法、语法分析阶段的错误（非法字符、括号未闭合等）
- `LispError`：求值阶段的错误（变量未定义、类型错误、参数数量错误等）

REPL 模式下两类异常均被捕获并打印到标准错误，解释器继续运行；文件模式下遇到异常以失败退出码退出。

---

## 测试

使用 `rjsj_test.hpp` 测试框架，通过以下所有测试集：

| 测试集 | 内容 |
|---|---|
| `Lv2` | 词法分析 |
| `Lv3` | 语法分析 |
| `Lv4` | 基础求值、变量定义 |
| `Lv5` | 内置过程调用 |
| `Lv5Extra` | 特殊形式 |
| `Lv6` | 闭包与词法作用域 |
| `Lv7` | 完整特殊形式 |
| `Lv7Lib` | 完整内置过程库 |
| `Sicp` | SICP 经典习题 |