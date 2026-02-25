#ifndef CHORD_CLI_H
#define CHORD_CLI_H

#include <string>
#include <vector>
#include <unordered_map>
#include "chord.h"

// 命令类型枚举
enum class CommandType
{
    UNKNOWN,
    HELP,
    EXIT,
    CLEAR,
    ADD_NODE,
    ADD_NODES,
    REMOVE_NODE,
    REMOVE_NODES,
    ADD_RESOURCE,
    ADD_RESOURCES,
    REMOVE_RESOURCE,
    REMOVE_RESOURCES,
    FIND_RESOURCE,
    FIND_RESOURCES,
    LIST_NODES,
    RING_STATUS,
    NODE_STATUS
};

// 命令解析结果
struct CommandResult
{
    CommandType type;
    std::vector<std::string> args;
    std::string error; // 错误信息（空表示无错误）
};

// CLI交互类（上帝视角操作台）
class ChordCLI
{
private:
    ChordRingManager &ringManager; // 关联的Chord环管理器
    bool is_running_;              // CLI运行状态

    // 命令语法定义：命令名 -> (参数数量, 语法说明)
    const std::unordered_map<std::string, std::pair<int, std::string>> command_syntax = {
        {"help", {0, "help - 显示所有命令帮助"}},
        {"exit", {0, "exit - 退出CLI控制台"}},
        {"clear", {0, "clear - 清空控制台屏幕"}},
        {"an", {1, "an <ip> - add_node(eg：an 192.168.1.101)"}},
        {"ans", {-1, "ans <ip1> <ip2> ... - add_nodes(eg：ans 192.168.1.101 192.168.1.102)"}},
        {"rn", {1, "rn <ip> - remove_node(eg：rn 192.168.1.101)"}},
        {"rns", {-1, "rns <ip1> <ip2> ... | * - remove_nodes(eg：rns 192.168.1.101 192.168.1.102 或 rns *)"}},
        {"ar", {1, "ar <name> - add_resource(eg：ar document.pdf)"}},
        {"ars", {-1, "ars <name1> <name2> ... - add_resources(eg：ars doc1.pdf doc2.pdf)"}},
        {"rr", {1, "rr <name> - remove_resource(eg：rr document.pdf)"}},
        {"rrs", {-1, "rrs <name1> <name2> ... | * - remove_resources(eg：rrs doc1.pdf doc2.pdf 或 rrs *)"}},
        {"fr", {1, "fr <name> - find_resource(eg：fr document.pdf)"}},
        {"frs", {-1, "frs <name1> <name2> ... - find_resources(eg：frs doc1.pdf doc2.pdf)"}},
        {"ln", {0, "ln - list_node"}},
        {"rs", {0, "rs - ring_status"}},
        {"ns", {1, "ns <ip> - node_status(eg：ns 192.168.1.101)"}},
    };

    // 私有方法：拆分命令行输入
    std::vector<std::string> split_command(const std::string &input);

    // 私有方法：验证命令参数
    CommandResult validate_command(const std::vector<std::string> &parts);

    // 私有方法：执行命令
    void execute_command(const CommandResult &cmd);

    // 工具函数
    static std::string trim(const std::string &s);
    static std::vector<std::string> split(const std::string &s, char delimiter);

public:
    explicit ChordCLI(ChordRingManager &ring);
    void run();
    void print_help();
    void print_error(const std::string &msg);
    void print_success(const std::string &msg);
};

#endif // CHORD_CLI_H