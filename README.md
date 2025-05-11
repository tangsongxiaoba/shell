# TASH

TASH (TAngsongxiaoba SHell) 是一个用 C 语言实现的简单的 Unix Shell 程序。

主要功能包括：

1. 基本的 Shell 功能：读取用户输入、解析命令和执行命令。
2. 支持执行外部命令和内置命令（cd 和 exit）。
3. 支持标准 I/O 重定向：输入重定向 (<)、输出重定向 (>) 和追加输出重定向 (>>)。
4. 支持管道 (|)：可以将一个命令的输出作为另一个命令的输入。

## 工作流程

### 基本流程

```mermaid
flowchart LR
    A[开始] --> B[显示欢迎信息]
    B --> C[进入tash_loop循环]
    C --> D[显示 tash> 提示符]
    D --> E[tash_read 读取用户输入]
    E --> F[tash_split 分割命令行]
    F --> G[tash_exec 执行命令]
    G --> H{执行成功?}
    H -- 是 --> I{是 exit 命令?}
    I -- 是 --> J[结束程序]
    I -- 否 --> C
    H -- 否 --> K[打印报错信息]
    K --> C
```

### 管道处理流程

管道 ('|') 的处理在 `tash_exec` 中完成。

```mermaid
flowchart LR
    A[tash_exec 开始] --> B{命令为空?}
    B -- 是 --> C[返回 STATUS_OK]
    B -- 否 --> D{是内建命令?}
    D -- 是 --> E[执行内建命令] --> V[返回状态码]
    D -- 否 --> I[根据管道符号分割出多个命令段]
    I --> J[循环处理每个命令段]
    J --> K[为当前命令创建管道]
    K --> L[fork创建子进程]
    L --> M{是子进程?}
    
    M -- 是 --> N[重定向管道输入输出]
    N --> P[调用 tash_prepare_exec 执行命令]
    
    M -- 否 --> Q[关闭不需要的管道端]
    Q --> R[保存管道读端作为下一个命令的输入]
    R --> S{还有更多命令?}
    S -- 是 --> J
    S -- 否 --> T[等待所有子进程结束]
    T --> U[返回STATUS_OK]
```

解析 `int tash_exec(char **args)` 对管道的处理：

1. **管道检测与分割**:
    - 初始化一个命令数组 `cmds`，第一个元素指向原始的 `args`。
    - 遍历 `args`，查找管道符 `|`。
    - 每当找到一个 `|`：
        - 将其替换为 `NULL`，从而将当前命令的参数列表截断。
        - 检查 `|` 后面是否有命令，或是否连续出现 `|`，若有则报语法错误。
        - 将 `|` 后的下一个参数地址存入 `cmds` 数组，作为下一个命令的开始。
2. **执行命令**:
    - 使用一个循环，遍历 `cmds` 数组中的每个命令。
    - `in_fd` 变量用于保存上一个命令的输出文件描述符（对于第一个命令是 `STDIN_FILENO`）。
    - **创建管道 (如果需要)**: 如果当前命令不是管道中的最后一个命令，则调用 `pipe()` 系统调用创建一个管道 `pipe_fds`。
    - **创建子进程**: 调用 `fork()` 系统调用创建一个子进程。 `pids[i]` 保存子进程ID。
    - 对于 **子进程 (`pids[i] == 0`)**:
        - **输入重定向**: 如果 `in_fd` 不是 `STDIN_FILENO`，则使用 `dup2(in_fd, STDIN_FILENO)` 将子进程的标准输入重定向到 `in_fd`。然后关闭 `in_fd`。
        - **输出重定向**: 如果当前命令不是最后一个命令，则：
            - 关闭当前管道的读端 `pipe_fds[0]`。
            - 使用 `dup2(pipe_fds[1], STDOUT_FILENO)` 将子进程的标准输出重定向到当前管道的写端 `pipe_fds[1]`。
            - 关闭 `pipe_fds[1]`。
        - 调用 `tash_prepare_exec(current_cmd_args)` 来处理该命令段自身的 I/O 重定向 (`<`, `>`, `>>`) 并最终执行命令。
    - 对于 **父进程 (`pids[i] > 0`)**:
        - **关闭不再需要的管道端口**:
            - 如果 `in_fd` 不是 `STDIN_FILENO`，关闭 `in_fd`。
            - 如果创建了管道，则关闭当前管道的写端 `pipe_fds[1]`。
            - 将 `in_fd` 更新为当前管道的读端 `pipe_fds[0]`，供下一个管道命令使用。
3. **等待子进程**: 在所有命令都已 `fork` 出去之后，父进程使用一个循环调用 `waitpid(pids[i], &child_status, 0)` 等待所有创建的子进程结束。

### 标准 I/O 重定向处理流程

标准 I/O 重定向 ('<', '>', '>>') 的处理在 `tash_prepare_exec` 中完成。

```mermaid
flowchart LR
    A[tash_prepare_exec 开始] --> B[初始化输入/输出文件指针]
    B --> C[解析命令行参数]
    
    C --> D{遇到 < 符号?}
    D -- 是 --> E[设置输入文件指针]
    
    C --> F{遇到 > 符号?}
    F -- 是 --> G[设置输出文件指针，不追加]
    
    C --> H{遇到 >> 符号?}
    H -- 是 --> I[设置输出文件指针，追加模式]
    
    C --> J[收集实际命令参数]
    J --> K{设置了输入文件指针?}
    K -- 是 --> L[打开输入文件]
    L --> M[重定向标准输入到文件]
    
    J --> N{设置了输出文件指针?}
    N -- 是 --> O[根据追加模式打开输出文件]
    O --> P[重定向标准输出到文件]
    
    J --> Q[使用 execvp 执行清理后的命令]
    Q --> R[如果执行失败，报错]
    R --> S[结束]
```

解析 `void tash_prepare_exec(char **args)` 对 I/O 重定向的处理：

1. 遍历原始参数数组 `args`：
    - 识别重定向符号：
        - `"<"`: 输入重定向。记录其后的参数为输入文件名。
        - `">"`: 输出重定向 (覆盖)。记录其后的参数为输出文件名，设置 `append_output` 为假。
        - `">>"`: 输出重定向 (追加)。记录其后的参数为输出文件名，设置 `append_output` 为真。
    - 将非重定向符号和它们的文件名参数的普通命令和参数复制到 `clean_args` 数组中。
2. **处理输入重定向**: 如果 `input_file` 被设置：
    - 使用 `open()` 系统调用以只读方式 (`O_RDONLY`) 打开输入文件。
    - 使用 `dup2()` 系统调用将打开文件的文件描述符复制到标准输入 (`STDIN_FILENO`)。
    - 使用 `close()` 系统调用关闭原始的文件描述符。
3. **处理输出重定向**: 如果 `output_file` 被设置：
    - 根据 `append_output` 标志，组合 `open()` 的 `flags` (`O_WRONLY | O_CREAT | O_APPEND` 或 `O_WRONLY | O_CREAT | O_TRUNC`)。
    - 使用 `open()` 系统调用打开或创建输出文件，权限设置为 `0644`。
    - 使用 `dup2()` 系统调用将打开文件的文件描述符复制到标准输出 (`STDOUT_FILENO`)。
    - 使用 `close()` 系统调用关闭原始的文件描述符。
3. 使用 `execvp()` 系统调用执行命令，并传递参数数组。`execvp` 会在 PATH 环境变量中查找命令。
4. 如果 `execvp` 返回 (表示执行失败)，则打印错误信息并使子进程异常退出。

## 实现原理

### 管道实现原理

```mermaid
sequenceDiagram
    participant Parent as 父进程
    participant Pipe as 管道
    participant Child1 as 子进程 1
    participant Child2 as 子进程 2
    
    Parent->>Pipe: 创建管道
    Parent->>Child1: fork 创建子进程 1
    Parent->>Child2: fork 创建子进程 2
    
    Child1->>Child1: 关闭管道读端
    Child1->>Child1: 将标准输出重定向到管道写端
    Child1->>Child1: 执行命令 1
    
    Child2->>Child2: 关闭管道写端
    Child2->>Child2: 将标准输入重定向到管道读端
    Child2->>Child2: 执行命令 2
    
    Parent->>Parent: 关闭所有管道端
    Parent->>Parent: 等待所有子进程结束
```

### 标准 I/O 重定向实现原理

```mermaid
flowchart LR
    A[用户命令<br>command < input.txt > output.txt] --> B[解析命令]
    B --> C[提取实际命令: command]
    B --> D[提取输入文件: input.txt]
    B --> E[提取输出文件: output.txt]
    
    D --> F[打开 input.txt 获取 fd_in]
    F --> G[dup2 将 fd_in 复制到 STDIN_FILENO]
    G --> H[关闭 fd_in]
    
    E --> I[打开 output.txt 获取 fd_out]
    I --> J[dup2 将 fd_out 复制到 STDOUT_FILENO]
    J --> K[关闭 fd_out]
    
    H --> L[执行实际命令 command]
    K --> L
```