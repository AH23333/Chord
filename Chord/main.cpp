// #include "chord.h"
// #include "logger.h"
// #include <iostream>
// #include <vector>
// #include <string>

// using namespace std;

// int main()
// {
//     logger.init("log.txt");
//     try
//     {
//         ChordRingManager *manager = new ChordRingManager();

//         vector<string> ips = {"192.168.1.125", "192.168.1.63", "192.168.1.15", "192.168.1.107", "192.168.1.33"};
//         vector<Chord *> chords;

//         for (auto &ip : ips)
//         {
//             cout << "\n创建节点: " << ip << endl;
//             Node node(ip);
//             cout << " -> " << node.toString() << endl;
//             Chord *chord = new Chord(node, &manager->getProxy());
//             manager->join(node, chord);
//             chord->joinRing();
//             chords.push_back(chord);
//             chord->showNodeInfo();
//         }

//         cout << "\n========== 添加资源 ==========" << endl;
//         vector<string> resources = {"file1.txt", "doc.docx", "img.jpg", "data.json", "cfg.xml"};
//         for (auto &res : resources)
//         {
//             manager->addResource(res);
//         }
//         manager->showResourceDistribution();

//         cout << "\n========== 添加新节点 ==========" << endl;
//         Node newNode("192.168.1.50");
//         Chord *newChord = new Chord(newNode, &manager->getProxy());
//         manager->join(newNode, newChord);
//         newChord->joinRing();
//         newChord->showNodeInfo();
//         manager->showResourceDistribution();

//         cout << "\n========== 测试离开 ==========" << endl;
//         Node leaveNode("192.168.1.63");
//         manager->removeNode(leaveNode);
//         manager->showResourceDistribution();

//         cout << "\n========== 最终状态 ==========" << endl;
//         manager->showChordInfo();

//         chords.clear();
//         delete manager;
//         logger.close();
//         return 0;
//     }
//     catch (const exception &e)
//     {
//         logger.error("错误: " + string(e.what()));
//         logger.close();
//         return 1;
//     }
// }

#include "chord.h"
#include "logger.h"
#include "chord_cli.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    logger.init("log.txt");
    try
    {
        ChordRingManager manager; // 使用默认 m=8
        ChordCLI cli(manager);
        cli.run();
        logger.close();
        return 0;
    }
    catch (const std::exception &e)
    {
        logger.error("错误: " + std::string(e.what()));
        logger.close();
        return 1;
    }
}