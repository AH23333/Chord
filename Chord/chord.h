#ifndef CHORD_H
#define CHORD_H

#include "node.h"
#include "config.h"
#include <vector>
#include <map>
#include <string>
#include <cstdint>

// 前置声明
class ChordRingManager;
class Chord;
class ChordProxy;

// 使用代理模式来管理 Chord 环，提供统一的接口，间接实现 ChordRingManager 的功能以达到类似节点之间的网络通信效果
class ChordProxy
{
private:
    ChordRingManager *ringManager;

public:
    // explicit 避免隐式转换
    explicit ChordProxy(ChordRingManager *manager);
    bool isRingEmpty();
    Node getAnyNodeInRing();
    std::vector<Node> getAllNodesInRing();
    Node findSuccessor(Node &currentNode, uint32_t id);
    Node findPredecessor(Node &currentNode, uint32_t id);
    bool transferResourceToNode(Node &targetNode, const std::string &resource);
    void notifyNodeUpdate(Node &updatedNode);
    Chord *findChordNodeByID(uint32_t id);
    std::vector<uint32_t> getAllSortedNodeIds();
    void notifyNodeLeave(Node &leftNode);
    Node findSuccessorFromAny(uint32_t id);
};

class ChordRingManager
{
private:
    std::map<uint32_t, Chord *> chordNodes;
    ChordProxy proxy;

public:
    ChordRingManager();
    ~ChordRingManager();
    bool join(Node &newNode, Chord *chordInstance);
    bool isRingEmpty() const;
    Node getAnyNode() const;
    std::map<uint32_t, Chord *> getAllChordNodes() const;
    Chord *findChordNode(uint32_t id);
    Node findSuccessor(Node &currentNode, uint32_t id);
    Node findPredecessor(Node &currentNode, uint32_t id);
    bool transferResourceToNode(Node &targetNode, const std::string &resource);
    void notifyAllNodesUpdate(Node &newNode);
    ChordProxy &getProxy();
    std::vector<uint32_t> getAllSortedNodeIds() const;
    int getTotalNodes() const;
    void notifyAllNodesLeave(Node &leftNode);
    bool removeNode(Node &leftNode);
    void showChordInfo() const;
    bool addResource(const std::string &resource);
    Node lookupResource(const std::string &resource);
    void showResourceDistribution();
    void refreshAllFingerTables();
    bool removeResource(const std::string &resourceName);
    std::vector<std::string> getAllResourceNames() const;

    // ===== 新增 CLI 辅助方法 =====
    bool join(const std::string &ip);               // 通过 IP 添加节点
    bool removeNodeByIP(const std::string &ip);     // 通过 IP 删除节点
    Node getNodeByIP(const std::string &ip) const;  // 通过 IP 获取节点（若不存在返回空 Node）
    bool nodeExists(const std::string &ip) const;   // 检查节点是否存在
    std::vector<std::string> getAllNodeIPs() const; // 获取所有节点 IP 列表
};

class Chord
{
private:
    Node self;
    Node predecessor;
    Node successor;
    std::vector<FingerEntry> fingerTable;
    ChordProxy *proxy;
    std::map<uint32_t, std::string> resources;

    void initAsFirstNode();
    bool isInInterval(uint32_t id, uint32_t start, uint32_t end);
    bool isInOpenInterval(uint32_t id, uint32_t start, uint32_t end);

public:
    Chord(Node self, ChordProxy *proxy);
    ~Chord();

    Node findSuccessor(uint32_t id);
    Node findClosestPrecedingNode(uint32_t id);
    Node findPredecessor(uint32_t id);
    void initWithBootstrapNode(Node &bootstrapNode);
    void initFingerTable();
    void notifyPredecessor(Node n);
    void notifyRelevantNodes();
    void updateFingerTable(Node &newNode);
    void fixFingers();
    void joinRing();
    void redistributeResources();
    bool transferResources();
    bool leaveRing();
    void handleNodeLeave(Node &leftNode);
    bool addResource(std::string resource);
    bool addResourceDirectly(uint32_t rid, const std::string &res);
    std::map<uint32_t, std::string> getAllResources() const;
    bool removeResourceDirectly(uint32_t rid);
    Node getSelf() const;
    Node getSuccessor() const;
    Node getPredecessor() const;
    int getResourceCount() const;
    void setPredecessor(const Node &n);
    void setSuccessor(const Node &n);
    void showNodeInfo() const;
};


#endif // CHORD_H
