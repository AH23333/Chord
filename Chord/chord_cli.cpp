#include "chord_cli.h"
#include "chord.h" // 确保 Chord 类型可见
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <limits>
#include <cstdlib>
#include <unordered_set>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

const std::unordered_map<std::string, CommandType> commandMap = {
    {"help", CommandType::HELP},
    {"exit", CommandType::EXIT},
    {"clear", CommandType::CLEAR},
    {"an", CommandType::ADD_NODE},
    {"ans", CommandType::ADD_NODES},
    {"rn", CommandType::REMOVE_NODE},
    {"rns", CommandType::REMOVE_NODES},
    {"ar", CommandType::ADD_RESOURCE},
    {"ars", CommandType::ADD_RESOURCES},
    {"rr", CommandType::REMOVE_RESOURCE},
    {"rrs", CommandType::REMOVE_RESOURCES},
    {"fr", CommandType::FIND_RESOURCE},
    {"frs", CommandType::FIND_RESOURCES},
    {"ln", CommandType::LIST_NODES},
    {"rs", CommandType::RING_STATUS},
    {"ns", CommandType::NODE_STATUS}};

// ---------------------- 工具函数 ----------------------

/**
 * @brief 移除字符串首尾的空格
 * @param s 输入字符串
 * @return 移除空格后的字符串
 */
string ChordCLI::trim(const string &s)
{
    auto start = s.begin();
    while (start != s.end() && isspace(static_cast<unsigned char>(*start)))
        ++start;
    auto end = s.end();
    if (start == end)
        return string();
    do
    {
        --end;
    } while (distance(start, end) > 0 && isspace(static_cast<unsigned char>(*end)));
    return string(start, end + 1);
}

/**
 * @brief 将字符串按指定分隔符分割为多个子字符串
 * @param s 输入字符串
 * @param delimiter 分隔符
 * @return 分割后的子字符串向量
 */
vector<string> ChordCLI::split(const string &s, char delimiter)
{
    vector<string> tokens;
    string token;
    istringstream tokenStream();
    while (getline(tokenStream, token, delimiter))
    {
        string trimmed = trim(token);
        if (!trimmed.empty())
            tokens.push_back(trimmed);
    }
    return tokens;
}

// ---------------------- ChordCLI类实现 ----------------------
ChordCLI::ChordCLI(ChordRingManager &ring) : ringManager(ring), is_running_(true) {}

/**
 * @brief 分割命令字符串为命令名和参数
 * @param input 输入命令字符串
 * @return 分割后的命令名和参数向量
 */
vector<string> ChordCLI::split_command(const string &input)
{
    return split(trim(input), ' ');
}

/**
 * @brief 验证命令是否有效
 * @param parts 命令名和参数向量
 * @return 验证结果
 */
CommandResult ChordCLI::validate_command(const vector<string> &parts)
{
    CommandResult result;
    result.type = CommandType::UNKNOWN;
    if (parts.empty())
        return result;

    string cmd = parts[0];

    // 使用commandMap查找命令类型
    auto cmdIt = commandMap.find(cmd);
    if (cmdIt != commandMap.end())
    {
        result.type = cmdIt->second;
    }
    else
    {
        result.error = "未知命令 '" + cmd + "'，输入 help 查看可用命令";
        return result;
    }

    auto it = command_syntax.find(cmd);
    if (it == command_syntax.end())
    {
        result.error = "命令 '" + cmd + "' 语法未定义";
        return result;
    }

    int required = it->second.first;
    int actual = static_cast<int>(parts.size() - 1);

    if (required == -1)
    {
        if (actual < 1)
        {
            result.error = "错误：命令 '" + cmd + "' 需要至少一个参数，语法：" + it->second.second;
            result.type = CommandType::UNKNOWN;
            return result;
        }
        for (size_t i = 1; i < parts.size(); ++i)
            result.args.push_back(parts[i]);
    }
    else
    {
        if (actual < required)
        {
            result.error = "错误：命令 '" + cmd + "' 缺少参数，语法：" + it->second.second;
            result.type = CommandType::UNKNOWN;
            return result;
        }
        for (int i = 1; i <= required; ++i)
            result.args.push_back(parts[i]);
    }

    if (cmd == "rns" && result.args.size() == 1 && result.args[0] == "*")
    {
        auto allIps = ringManager.getAllNodeIPs();
        if (allIps.empty())
        {
            result.error = "错误：当前环中无节点可删除";
            result.type = CommandType::UNKNOWN;
            return result;
        }
        result.args = allIps;
    }

    if (cmd == "rrs" && result.args.size() == 1 && result.args[0] == "*")
    {
        auto allResources = ringManager.getAllResourceNames();
        if (allResources.empty())
        {
            result.error = "错误：当前环中无资源可删除";
            result.type = CommandType::UNKNOWN;
            return result;
        }
        result.args = allResources;
    }

    if ((cmd == "rn" || cmd == "ns") && result.args.size() >= 1)
    {
        const string &ip = result.args[0];
        if (!ringManager.nodeExists(ip))
        {
            result.error = "错误：节点IP '" + ip + "' 不存在";
            result.type = CommandType::UNKNOWN;
            return result;
        }
    }

    return result;
}

void ChordCLI::execute_command(const CommandResult &cmd)
{
    switch (cmd.type)
    {
    case CommandType::HELP:
        print_help();
        break;

    case CommandType::EXIT:
        is_running_ = false;
        print_success("正在退出CLI控制台...");
        break;

    case CommandType::CLEAR:
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
        print_success("屏幕已清空");
        break;

    case CommandType::ADD_NODE:
    {
        const string &ip = cmd.args[0];
        if (ringManager.join(ip))
            print_success("节点 " + ip + " 添加成功");
        else
            print_error("节点 " + ip + " 添加失败（可能已存在或内部错误）");
        break;
    }

    case CommandType::ADD_NODES:
    {
        vector<string> success, fail;
        for (const string &ip : cmd.args)
        {
            if (ringManager.join(ip))
                success.push_back(ip);
            else
                fail.push_back(ip);
        }
        stringstream ss;
        if (!success.empty())
        {
            ss << "成功添加节点：";
            for (size_t i = 0; i < success.size(); ++i)
                ss << success[i] << (i + 1 < success.size() ? ", " : "");
            print_success(ss.str());
        }
        ss.str("");
        if (!fail.empty())
        {
            ss << "添加失败（节点已存在）：";
            for (size_t i = 0; i < fail.size(); ++i)
                ss << fail[i] << (i + 1 < fail.size() ? ", " : "");
            print_error(ss.str());
        }
        break;
    }

    case CommandType::REMOVE_NODE:
    {
        const string &ip = cmd.args[0];
        // 检查是否删除最后一个节点
        auto allIps = ringManager.getAllNodeIPs();
        if (allIps.size() == 1 && allIps[0] == ip)
        {
            cout << "\033[33m[警告] 即将删除最后一个节点，所有资源将永久丢失！确认删除？(y/n)\033[0m ";
            string confirm;
            getline(cin, confirm);
            if (confirm != "y" && confirm != "Y")
            {
                print_success("已取消删除操作");
                break;
            }
        }
        if (ringManager.removeNodeByIP(ip))
            print_success("节点 " + ip + " 移除成功");
        else
            print_error("节点 " + ip + " 移除失败（可能不存在或内部错误）");
        break;
    }

    case CommandType::REMOVE_NODES:
    {
        auto allIps = ringManager.getAllNodeIPs();
        // 先去重用户输入的IP，用于判断是否删除所有节点
        unordered_set<std::string> uniqueArgs(cmd.args.begin(), cmd.args.end());
        bool isRemoveAll = true;
        if (allIps.size() != uniqueArgs.size())
            isRemoveAll = false;
        else
        {
            for (const string &ip : allIps)
            {
                if (uniqueArgs.find(ip) == uniqueArgs.end())
                {
                    isRemoveAll = false;
                    break;
                }
            }
        }

        if (isRemoveAll && !allIps.empty())
        {
            cout << "\033[33m[警告] 即将删除所有节点，所有资源将永久丢失！确认删除？(y/n)\033[0m ";
            string confirm;
            getline(cin, confirm);
            if (confirm != "y" && confirm != "Y")
            {
                print_success("已取消删除操作");
                break;
            }
        }

        vector<string> success, fail;
        for (const string &ip : cmd.args)
        {
            if (ringManager.removeNodeByIP(ip))
                success.push_back(ip);
            else
                fail.push_back(ip);
        }
        stringstream ss;
        if (!success.empty())
        {
            ss << "成功移除节点：";
            for (size_t i = 0; i < success.size(); ++i)
                ss << success[i] << (i + 1 < success.size() ? ", " : "");
            print_success(ss.str());
        }
        ss.str("");
        if (!fail.empty())
        {
            ss << "移除失败（节点不存在）：";
            for (size_t i = 0; i < fail.size(); ++i)
                ss << fail[i] << (i + 1 < fail.size() ? ", " : "");
            print_error(ss.str());
        }
        break;
    }

    case CommandType::ADD_RESOURCE:
    {
        const string &name = cmd.args[0];
        if (ringManager.addResource(name))
            print_success("资源 '" + name + "' 添加成功");
        else
            print_error("资源 '" + name + "' 添加失败（可能环为空或资源已存在？）");
        break;
    }

    case CommandType::ADD_RESOURCES:
    {
        vector<string> success, fail;
        for (const string &name : cmd.args)
        {
            if (ringManager.addResource(name))
                success.push_back(name);
            else
                fail.push_back(name);
        }
        stringstream ss;
        if (!success.empty())
        {
            ss << "成功添加资源：";
            for (size_t i = 0; i < success.size(); ++i)
                ss << success[i] << (i + 1 < success.size() ? ", " : "");
            print_success(ss.str());
        }
        ss.str("");
        if (!fail.empty())
        {
            ss << "添加失败（环空或重复）：";
            for (size_t i = 0; i < fail.size(); ++i)
                ss << fail[i] << (i + 1 < fail.size() ? ", " : "");
            print_error(ss.str());
        }
        break;
    }

    case CommandType::REMOVE_RESOURCE:
    {
        const string &name = cmd.args[0];
        if (ringManager.getTotalNodes() == 0)
        {
            print_error("环为空，无法删除资源");
            break;
        }
        if (ringManager.removeResource(name))
            print_success("资源 '" + name + "' 删除成功");
        else
            print_error("资源 '" + name + "' 删除失败（可能不存在或内部错误）");
        break;
    }

    case CommandType::REMOVE_RESOURCES:
    {
        if (ringManager.getTotalNodes() == 0)
        {
            print_error("环为空，无法删除资源");
            break;
        }
        auto allResources = ringManager.getAllResourceNames();
        bool isRemoveAll = true;
        for (const string &name : allResources)
        {
            if (find(cmd.args.begin(), cmd.args.end(), name) == cmd.args.end())
            {
                isRemoveAll = false;
                break;
            }
        }
        if (isRemoveAll && !allResources.empty())
        {
            cout << "\033[33m[警告] 即将删除所有资源，此操作不可恢复！确认删除？(y/n)\033[0m ";
            string confirm;
            getline(cin, confirm);
            if (confirm != "y" && confirm != "Y")
            {
                print_success("已取消删除操作");
                break;
            }
        }

        vector<string> success, fail;
        for (const string &name : cmd.args)
        {
            if (ringManager.removeResource(name))
                success.push_back(name);
            else
                fail.push_back(name);
        }
        stringstream ss;
        if (!success.empty())
        {
            ss << "成功删除资源：";
            for (size_t i = 0; i < success.size(); ++i)
                ss << success[i] << (i + 1 < success.size() ? ", " : "");
            print_success(ss.str());
        }
        ss.str("");
        if (!fail.empty())
        {
            ss << "删除失败（资源不存在）：";
            for (size_t i = 0; i < fail.size(); ++i)
                ss << fail[i] << (i + 1 < fail.size() ? ", " : "");
            print_error(ss.str());
        }
        break;
    }

    case CommandType::FIND_RESOURCE:
    {
        const string &name = cmd.args[0];
        if (ringManager.getTotalNodes() == 0)
        {
            print_error("环为空，无法查找资源");
            break;
        }
        Node n = ringManager.lookupResource(name);
        if (n.isEmpty())
            print_error("资源 '" + name + "' 不存在");
        else
            print_success("资源 '" + name + "' 由节点 " + n.toString() + " 负责");
        break;
    }

    case CommandType::FIND_RESOURCES:
    {
        if (ringManager.getTotalNodes() == 0)
        {
            print_error("环为空，无法查找资源");
            break;
        }
        print_success("批量查找资源结果：");
        stringstream ss;
        for (const string &name : cmd.args)
        {
            Node n = ringManager.lookupResource(name);
            if (n.isEmpty())
            {
                ss << "  资源 '" << name << "' 不存在";
                print_error(ss.str());
            }
            else
            {
                ss << "  资源 '" << name << "' 由节点 " << n.toString() << " 负责";
                print_success(ss.str());
            }
            ss.str("");
        }
        break;
    }

    case CommandType::LIST_NODES:
    {
        auto ids = ringManager.getAllSortedNodeIds();
        auto ips = ringManager.getAllNodeIPs();
        if (ips.empty())
            print_success("当前环中无节点");
        else
        {
            print_success("当前环中节点IP列表：");
            for (size_t i = 0; i < ips.size(); ++i)
                cout << "  " << (i + 1) << ". ID: " << ids[i] << " -> IP: " << ips[i] << endl;
        }
        break;
    }

    case CommandType::RING_STATUS:
        ringManager.showChordInfo();
        ringManager.showResourceDistribution();
        break;

    case CommandType::NODE_STATUS:
    {
        const string &ip = cmd.args[0];
        Node node = ringManager.getNodeByIP(ip);
        if (node.isEmpty())
            print_error("节点 " + ip + " 不存在");
        else
        {
            // 使用 auto 推导类型，避免显式 Chord* 可能引起的解析问题
            auto pChord = ringManager.findChordNode(node.id);
            if (pChord)
                pChord->showNodeInfo();
            else
                print_error("内部错误：无法获取节点对象");
        }
        break;
    }

    default:
        break;
    }
}

void ChordCLI::run()
{
    print_success("=== Chord CLI ===");
    print_success("输入 help 查看所有命令，exit 退出");
    cout << endl;

    while (is_running_)
    {
        cout << "chord> ";
        string input;
        getline(cin, input);

        if (input.empty())
            continue;

        auto parts = split_command(input);
        auto cmd_result = validate_command(parts);

        if (!cmd_result.error.empty())
        {
            print_error(cmd_result.error);
            continue;
        }

        execute_command(cmd_result);
        cout << endl;
    }
}

void ChordCLI::print_help()
{
    print_success("=== 可用命令列表 ===");
    for (const auto &pair : command_syntax)
        cout << "  " << pair.second.second << endl;
}

void ChordCLI::print_error(const string &msg)
{
    cout << "\033[31m[错误] " << msg << "\033[0m" << endl;
}

void ChordCLI::print_success(const string &msg)
{
    cout << "\033[32m[成功] " << msg << "\033[0m" << endl;

}

