#include "chord.h"
#include "logger.h"
#include "SHA_1.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

using namespace std;

/*一点小巧思
前置条件
假设Chord环为圆O，圆心为O点，以O点竖直向上与圆相交的点H为哈希值为0点
约定以OH为始边，顺时针方向为正
节点-圆心连线与OH夹角在(0,90)度为右侧，(90,180)度为左侧

假设节点A落在环上右侧，与OH夹角为锐角α_1，
A的前驱节点为B在环的左侧，与OH夹角为角α_2(180 < α_2 < 360)，
A的后继节点为C在环的右侧，与OH夹角为锐角α_3(α_3 > α_1)，
剩余节点分布在弧CB之间，不确定有多少个节点

那么由于A的加入，使得弧AC间角度为α_3 - α_1，
弧AC上左开右闭区(A,C]间的id对应的finger table不需要修改，
实际需要修改的是弧BA经过旋转所覆盖的id区间的finger table，
旋转角度为180、90、45、22.5 ... 0的等比数列，数列长度取决于m的大小

本例中需要更新的节点区间应该是角度为(0,α_1]U(α_2,360] U [][][]。。。摆烂了，这个角度还是懒得算了

换一个例子，直接看新加入环的A点，和前驱节点B的弧BA形成的夹角为α，若以OA为始边，逆时针为正方向，则需要更新节点的角度区间为[180-α,180)U[90-α,90)U[45-α,45)U[22.5-α,22.5)U[11.25-α,11.25)U...
由于实际上环上id是m位二进制数，实际还算好弄
nId = 新加入节点n的m位二进制id
pId = 节点n的前驱p的m位二进制id
distance = 新加入节点n与前驱节点p的id距离
if(nId < pId) distance = ID_SPACE - pId + nId;
else distance = nId - pId;

for (int i = 0; i < m; i++)
{
    startId = (pId - (1 << i)) % ID_SPACE - 1;
    endId = (updateStartId + distance) % ID_SPACE;
    updateFingerTable(startId, endId);
}

但还是太麻烦了不如直接更新所有节点的finger table来的简单痛快，毕竟这个也没几个节点
*/

// ==================== ChordProxy 实现 ====================

ChordProxy::ChordProxy(ChordRingManager *manager) : ringManager(manager)
{
    logger.info("ChordProxy 初始化");
}

bool ChordProxy::isRingEmpty() { return ringManager ? ringManager->isRingEmpty() : true; }

/**
 * @brief 获取Chord环中的任意节点（用于初始化）
 * @return Node 任意Chord节点，若为空则返回空节点
 */
Node ChordProxy::getAnyNodeInRing()
{
    return ringManager ? ringManager->getAnyNode() : Node();
}

/**
 * @brief 获取Chord环中的所有节点
 * @return vector<Node> 所有Chord节点的向量
 */
vector<Node> ChordProxy::getAllNodesInRing()
{
    vector<Node> nodes;
    if (!ringManager)
        return nodes;
    for (auto &p : ringManager->getAllChordNodes())
    {
        nodes.push_back(p.second->getSelf());
    }
    return nodes;
}

/**
 * @brief 通过特定的当前节点查找Chord环中指定ID的节点的后继节点
 * @param currentNode 当前节点（用于定位Chord环）
 * @param id 目标节点ID
 * @return Node 目标节点的后继节点，若不存在则返回空节点
 */
Node ChordProxy::findSuccessor(Node &currentNode, uint32_t id)
{
    Chord *chord = findChordNodeByID(currentNode.id);
    if (chord)
        return chord->findSuccessor(id);
    return findSuccessorFromAny(id);
}

/**
 * @brief 通过特定的当前节点查找Chord环中指定ID的节点的前驱节点
 * @param currentNode 当前节点（用于定位Chord环）
 * @param id 目标节点ID
 * @return Node 目标节点的前驱节点，若不存在则返回空节点
 */
Node ChordProxy::findPredecessor(Node &currentNode, uint32_t id)
{
    Chord *chord = findChordNodeByID(currentNode.id);
    return chord ? chord->findPredecessor(id) : Node();
}

/**
 * @brief 向Chord环中的指定节点传输资源
 * @param targetNode 目标节点（用于定位Chord环）
 * @param resource 要传输的资源字符串
 * @return true 若成功传输
 * @return false 若传输失败（节点不存在或其他原因）
 */
bool ChordProxy::transferResourceToNode(Node &targetNode, const string &resource)
{
    Chord *chord = findChordNodeByID(targetNode.id);
    if (!chord)
        return false;
    try
    {
        return chord->addResourceDirectly(sha1_hash_to_uint32(resource) % ID_SPACE, resource);
    }
    catch (...)
    {
        return false;
    }
}

/**
 * @brief 通知Chord环中的所有节点当前节点的更新
 * @param updatedNode 当前节点（用于定位Chord环）
 */
void ChordProxy::notifyNodeUpdate(Node &updatedNode)
{
    if (ringManager)
        ringManager->notifyAllNodesUpdate(updatedNode);
}

/**
 * @brief 根据ID查找Chord环中的节点
 * @param id 目标节点ID
 * @return Chord* 指向目标节点的指针，若不存在则返回nullptr
 */
Chord *ChordProxy::findChordNodeByID(uint32_t id)
{
    return ringManager ? ringManager->findChordNode(id) : nullptr;
}

/**
 * @brief 获取所有节点ID，按ID排序
 * @return vector<uint32_t> 所有节点ID的向量，按ID升序排序
 */
vector<uint32_t> ChordProxy::getAllSortedNodeIds()
{
    return ringManager ? ringManager->getAllSortedNodeIds() : vector<uint32_t>();
}

/**
 * @brief 通知Chord环中的所有节点当前节点的离开
 * @param leftNode 当前节点（用于定位Chord环）
 */
void ChordProxy::notifyNodeLeave(Node &leftNode)
{
    if (ringManager)
        ringManager->notifyAllNodesLeave(leftNode);
}

/**
 * @brief 从Chord环中查找指定ID的节点的后继节点（任意节点）
 * @param id 目标节点ID
 * @return Node 目标节点的后继节点，若不存在则返回空节点
 */
Node ChordProxy::findSuccessorFromAny(uint32_t id)
{
    if (!ringManager || ringManager->isRingEmpty())
    {
        return Node();
    }
    vector<uint32_t> sortedIds = getAllSortedNodeIds();
    int n = sortedIds.size();
    if (n == 0)
    {
        return Node();
    }
    if (n == 1)
    {
        Chord *chord = findChordNodeByID(sortedIds[0]);
        return chord ? chord->getSelf() : Node();
    }
    for (int i = 0; i < n; i++)
    {
        if (sortedIds[i] >= id)
        {
            Chord *chord = findChordNodeByID(sortedIds[i]);
            return chord ? chord->getSelf() : Node();
        }
    }
    Chord *chord = findChordNodeByID(sortedIds[0]);
    return chord ? chord->getSelf() : Node();
}

// ==================== ChordRingManager 实现 ====================

ChordRingManager::ChordRingManager() : proxy(this)
{
    logger.info("ChordRingManager 初始化");
}

ChordRingManager::~ChordRingManager()
{
    logger.info("ChordRingManager 析构");
    for (auto &p : chordNodes)
        delete p.second;
    chordNodes.clear();
}

/**
 * @brief 加入Chord环
 * @param newNode 新节点
 * @param chordInstance Chord实例指针
 * @return true 若成功加入
 * @return false 若失败（节点已存在或其他原因）
 */
bool ChordRingManager::join(Node &newNode, Chord *chordInstance)
{
    if (findChordNode(newNode.id))
    {
        logger.warning("节点已存在: " + newNode.toString());
        return false;
    }
    chordNodes[newNode.id] = chordInstance;
    logger.info("节点加入: " + newNode.toString());

    // 实际应该是每个节点都会周期性刷新finger table保持稳定，范围是半个环上的节点，这里在每个节点加入后更新全部节点的finger table保证稳定
    refreshAllFingerTables();
    return true;
}

/**
 * @brief 检查Chord环是否为空
 * @return true 若为空
 * @return false 若不为空
 */
bool ChordRingManager::isRingEmpty() const { return chordNodes.empty(); }

/**
 * @brief 获取任意Chord节点（用于初始化）
 * @return Node 任意Chord节点，若为空则返回空节点
 */
Node ChordRingManager::getAnyNode() const
{
    return chordNodes.empty() ? Node() : chordNodes.begin()->second->getSelf();
}

/**
 * @brief 获取所有Chord节点（ID到Chord实例的映射）
 * @return map<uint32_t, Chord *> 所有Chord节点的映射
 */
map<uint32_t, Chord *> ChordRingManager::getAllChordNodes() const
{
    return chordNodes;
}

/**
 * @brief 根据ID查找Chord节点
 * @param id 节点ID
 * @return Chord* 找到的Chord节点指针，若不存在则返回nullptr
 */
Chord *ChordRingManager::findChordNode(uint32_t id)
{
    auto it = chordNodes.find(id);
    return (it != chordNodes.end()) ? it->second : nullptr;
}

/**
 * @brief 通过当前节点根据ID查找Chord节点的直接后继节点
 * @param currentNode 当前节点（用于定位Chord环）
 * @param id 节点ID
 * @return Node 找到的Chord节点的直接后继节点，若不存在则返回空节点
 */
Node ChordRingManager::findSuccessor(Node &currentNode, uint32_t id)
{
    Chord *chord = findChordNode(currentNode.id);
    return chord ? chord->findSuccessor(id) : currentNode;
}

/**
 * @brief 通过当前节点根据ID查找Chord节点的直接前驱节点
 * @param currentNode 当前节点（用于定位Chord环）
 * @param id 节点ID
 * @return Node 找到的Chord节点的直接前驱节点，若不存在则返回空节点
 */
Node ChordRingManager::findPredecessor(Node &currentNode, uint32_t id)
{
    Chord *chord = findChordNode(currentNode.id);
    return chord ? chord->findPredecessor(id) : Node();
}

/**
 * @brief 将资源传输到指定ID的Chord节点
 * @param targetNode 目标节点（接收资源的节点）
 * @param resource 资源内容
 * @return true 若成功传输
 * @return false 若失败（目标节点不存在或其他原因）
 */
bool ChordRingManager::transferResourceToNode(Node &targetNode, const string &resource)
{
    Chord *chord = findChordNode(targetNode.id);
    return chord ? chord->addResourceDirectly(sha1_hash_to_uint32(resource) % ID_SPACE, resource) : false;
}

/**
 * @brief 通知所有Chord节点更新新节点的finger table
 * @param newNode 新加入的节点（用于更新其他节点的finger table）
 */
void ChordRingManager::notifyAllNodesUpdate(Node &newNode)
{
    if (chordNodes.empty() || newNode.isEmpty())
        return;
    for (auto &p : chordNodes)
    {
        if (p.first != newNode.id)
        {
            p.second->updateFingerTable(newNode);
        }
    }
}

/**
 * @brief 获取Chord环管理器的代理对象
 * @return ChordProxy& Chord环管理器的代理对象引用
 */
ChordProxy &ChordRingManager::getProxy() { return proxy; }

/**
 * @brief 获取所有Chord节点的ID，按ID排序
 * @return vector<uint32_t> 所有Chord节点的ID的向量，按ID升序排序
 */
vector<uint32_t> ChordRingManager::getAllSortedNodeIds() const
{
    if (chordNodes.empty())
    {
        return vector<uint32_t>();
    }
    vector<uint32_t> ids;
    for (auto &p : chordNodes)
        ids.push_back(p.first);
    sort(ids.begin(), ids.end());
    return ids;
}

/**
 * @brief 获取所有Chord节点的数量
 * @return int 所有Chord节点的数量
 */
int ChordRingManager::getTotalNodes() const { return chordNodes.size(); }

/**
 * @brief 通知所有Chord节点有节点离开
 * @param leftNode 离开的Chord节点
 */
void ChordRingManager::notifyAllNodesLeave(Node &leftNode)
{
    if (chordNodes.empty() || leftNode.isEmpty())
        return;
    for (auto &p : chordNodes)
    {
        if (p.first != leftNode.id)
        {
            p.second->handleNodeLeave(leftNode);
        }
    }
}

/**
 * @brief 移除Chord节点
 * @param leftNode 离开的Chord节点
 * @return true 若成功移除
 * @return false 若失败（节点不存在或其他原因）
 */
bool ChordRingManager::removeNode(Node &leftNode)
{
    auto it = chordNodes.find(leftNode.id);
    if (it == chordNodes.end())
    {
        logger.warning("节点不存在: " + leftNode.toString());
        return false;
    }

    Chord *chord = it->second;
    bool result = chord->leaveRing();

    // 通知所有节点有节点离开（无奈之举了属于是）
    notifyAllNodesLeave(leftNode);
    chordNodes.erase(it);

    // 删除节点后也通知所有节点更新 finger table（也是无奈之举，太弱小了）
    refreshAllFingerTables();

    delete chord;
    logger.info("节点移除: " + leftNode.toString());
    return result;
}

/**
 * @brief 显示Chord环信息
 */
void ChordRingManager::showChordInfo() const
{
    logger.info("========== Chord Ring ==========");
    logger.info("节点数: " + to_string(chordNodes.size()));
    for (auto &p : chordNodes)
    {
        p.second->showNodeInfo();
    }
}

/**
 * @brief 添加资源到Chord环
 * @param resource 资源名称
 * @return true 若成功添加
 * @return false 若失败（环为空或其他原因）
 */
bool ChordRingManager::addResource(const string &resource)
{
    if (chordNodes.empty())
        return false;
    uint32_t rid = sha1_hash_to_uint32(resource) % ID_SPACE;

    // 尝试获取任意节点作为起始点
    Node anyNode = getAnyNode();
    if (anyNode.isEmpty())
        return false;

    // 直接把节点作为入口，查找负责节点
    Node responsible = findSuccessor(anyNode, rid);
    Chord *chord = findChordNode(responsible.id);
    if (!chord)
        return false;
    bool result = chord->addResource(resource);
    if (result)
        logger.info("资源 '" + resource + "' -> " + responsible.toString());
    return result;
}

/**
 * @brief 查找资源的Chord节点
 * @param resource 资源名称
 * @return Node 负责存储该资源的Chord节点的Node信息，若不存在则返回空Node节点
 */
Node ChordRingManager::lookupResource(const string &resource)
{
    if (chordNodes.empty())
        return Node();
    uint32_t rid = sha1_hash_to_uint32(resource) % ID_SPACE;
    Node anyNode = getAnyNode();
    if (anyNode.isEmpty())
        return Node();

    Node responsible = findSuccessor(anyNode, rid);
    Chord *chord = findChordNode(responsible.id);
    if (chord)
    {
        // 检查该节点是否真正拥有这个资源
        const auto &resources = chord->getAllResources();
        if (resources.find(rid) != resources.end())
            return responsible; // 资源存在，返回负责节点
    }

    return Node();
}

/**
 * @brief 显示资源分布
 */
void ChordRingManager::showResourceDistribution()
{
    cout << "\n========== 资源分布 ==========" << endl;
    int total = 0;
    for (auto &p : chordNodes)
    {
        int count = p.second->getResourceCount();
        total += count;
        cout << p.second->getSelf().toString() << ": " << count << endl;
    }
    cout << "总计: " << total << endl;
    cout << "==============================" << endl;
}

/**
 * @brief 刷新所有Chord节点的finger table
 */
void ChordRingManager::refreshAllFingerTables()
{
    for (auto &p : chordNodes)
        p.second->fixFingers();
}

// ==================== Chord 实现 ====================

Chord::Chord(Node self, ChordProxy *proxy)
    : self(self), predecessor(Node()), successor(Node()), proxy(proxy)
{
    fingerTable.resize(m);
    for (uint32_t i = 0; i < m; i++)
    {
        fingerTable[i].startId = (self.id + (1u << i)) % ID_SPACE;
        fingerTable[i].node = self;
    }
    string fingerTableStr = "[";
    for (const auto &entry : fingerTable)
    {
        fingerTableStr += "(" + to_string(entry.startId) + ", " + entry.node.toString() + "), ";
    }
    fingerTableStr += "]";
    logger.info("Init Chord " + self.toString() + " with finger table: " + fingerTableStr);
}

Chord::~Chord()
{
    logger.info("Destroy Chord " + self.toString());
    resources.clear();
    fingerTable.clear();
}

/**
 * @brief 初始化Chord环的第一个节点
 */
void Chord::initAsFirstNode()
{
    predecessor = self;
    successor = self;
    for (int i = 0; i < m; i++)
        fingerTable[i].node = self;
    logger.info("Init Chord " + self.toString() + " as the first node in the ring.");
}

/**
 * @brief 检查ID是否在闭区间(start, end]内
 * @param id 要检查的ID
 * @param start 区间起始ID（不包含）
 * @param end 区间结束ID（包含）
 * @return true 若ID在闭区间内
 * @return false 若ID不在闭区间内
 */
bool Chord::isInInterval(uint32_t id, uint32_t start, uint32_t end)
{
    if (start == end)
        return id != start;
    if (start < end)
        return id > start && id <= end;
    return id > start || id <= end;
}

/**
 * @brief 检查ID是否在开区间(start, end)内
 * @param id 要检查的ID
 * @param start 区间起始ID（不包含）
 * @param end 区间结束ID（不包含）
 * @return true 若ID在开区间内
 * @return false 若ID不在开区间内
 */
bool Chord::isInOpenInterval(uint32_t id, uint32_t start, uint32_t end)
{
    if (start == end)
        return false;
    if (start < end)
        return id > start && id < end;
    return id > start || id < end;
}

/**
 * @brief 查找Chord环中节点为id的后继节点
 * @param id 要查找的节点ID
 * @return Node 后继节点，若不存在则返回空节点
 */
Node Chord::findSuccessor(uint32_t id)
{
    if (successor.isEmpty())
        return self;

    if (id == self.id)
        return self;

    if (isInInterval(id, self.id, successor.id))
        return successor;

    if (!predecessor.isEmpty() && isInInterval(id, predecessor.id, self.id))
    {
        return self;
    }

    // 如果在finger table中，则查找该节点的前继节点
    Node closest = findClosestPrecedingNode(id);
    if (closest.id == self.id)
    {
        return successor;
    }

    // 否则，通过网络（代理）查找该节点的后继节点
    if (proxy)
    {
        Chord *closestChord = proxy->findChordNodeByID(closest.id);
        if (closestChord)
            return closestChord->findSuccessor(id);
    }
    return successor;
}

/**
 * @brief 在finger table中查找Chord环中节点id的最接近的前驱节点
 * @param id 要查找的节点ID
 * @return Node 最接近的前驱节点，若不存在则返回空节点
 */
Node Chord::findClosestPrecedingNode(uint32_t id)
{
    for (int i = m - 1; i >= 0; i--)
    {
        Node &fingerNode = fingerTable[i].node;
        if (fingerNode.isEmpty() || fingerNode == self)
            continue;

        if (isInInterval(fingerNode.id, self.id, id))
        {
            if (fingerNode.id != id)
                return fingerNode;
        }
    }
    return self;
}

/**
 * @brief 查找Chord环中ID为id的节点的前驱节点
 * @param id 要查找的节点ID
 * @return Node 前驱节点，若查找失败则返回当前最佳猜测节点
 */
Node Chord::findPredecessor(uint32_t id)
{
    // 1.检查网络代理状态
    // 如果没有代理（网络不可用），说明当前节点是孤立节点
    // 在孤立节点情况下，自身就是自己的前驱
    if (!proxy)
    {
        logger.warning("findPredecessor: proxy is null, returning self " + self.toString() + " as predecessor");
        return self;
    }

    // 2.特殊情况处理 - 查找自己的前驱
    // 如果要查找的节点就是自己，直接返回记录的前驱节点
    // 如果没有前驱记录（单节点情况），返回自身
    if (id == self.id)
    {
        return predecessor.isEmpty() ? self : predecessor;
    }

    // 3.初始化查找状态
    Node current = self;        // 当前正在检查的节点，从自身开始
    int hops = 0;               // 已进行的跳数计数
    const int MAX_HOPS = m * 2; // 最大跳数限制：2倍标识符位数，防止无限循环，实际上最多只需要遍历m次即可找到前驱节点，因为每个节点的finger table中最多只有m个节点，但考虑到实际中的网络延迟以及经验，这里设置为2m

    // 4.查找主循环
    // 在最大跳数限制内进行查找
    while (hops++ < MAX_HOPS)
    {
        // 获取当前节点的Chord对象
        // 如果是自身节点，直接使用this指针
        // 如果是其他节点，通过代理查找对应的Chord对象
        Chord *currentChord = (current.id == self.id) ? this : proxy->findChordNodeByID(current.id);

        // 如果无法找到当前节点的Chord对象，说明节点可能已离开环
        if (!currentChord)
        {
            logger.warning("findPredecessor: cannot find chord node for " + current.toString());
            break;
        }

        // 获取当前节点的后继节点
        Node succ = currentChord->getSuccessor();

        // 如果后继节点为空，说明环结构可能有问题
        if (succ.isEmpty())
        {
            logger.warning("findPredecessor: successor is empty for " + current.toString());
            break;
        }

        // 检查是否找到前驱
        // 关键条件：如果目标id在(current.id, succ.id]区间内
        // 那么current就是id的前驱节点
        if (isInInterval(id, current.id, succ.id))
        {
            // 找到前驱节点，返回结果
            return current;
        }

        // 向前推进到更接近的节点
        // 在当前节点的finger table中查找最接近目标id的前驱节点
        Node next = currentChord->findClosestPrecedingNode(id);

        // 检查是否需要退出循环
        // 如果找不到更接近的节点，或者找到的节点就是当前节点
        // 说明已经到达查找的终点
        if (next.id == current.id || next.isEmpty())
        {
            logger.info("findPredecessor: no closer node found, current hop: " + to_string(hops));
            break;
        }

        // 更新当前节点，继续下一轮查找
        current = next;
    }

    // 5.查找结束处理
    // 如果达到最大跳数仍未找到，返回当前最佳猜测节点
    // 并记录警告日志
    logger.warning("findPredecessor: max hops reached, returning current node " + current.toString());
    return current;
}

/**
 * @brief 初始化Chord环中的节点，使其加入到以bootstrapNode为引导节点的环中
 * @param bootstrapNode 引导节点，若为空则初始化第一个节点
 */
void Chord::initWithBootstrapNode(Node &bootstrapNode)
{
    logger.info("initWithBootstrapNode: self=" + self.toString() + ", bootstrap=" + bootstrapNode.toString());
    if (bootstrapNode.isEmpty() || bootstrapNode == self)
    {
        initAsFirstNode();
        return;
    }

    try
    {
        Chord *bootstrapChord = proxy->findChordNodeByID(bootstrapNode.id);
        if (!bootstrapChord)
            throw runtime_error("无法获取引导节点");
        Node successorNode = bootstrapChord->findSuccessor(self.id);
        if (successorNode.isEmpty())
            throw runtime_error("无法找到后继");

        logger.info("找到后继: " + successorNode.toString() + ", bootstrap=" + bootstrapNode.toString() + ", successorNode == bootstrapNode: " + string(successorNode == bootstrapNode ? "true" : "false"));

        successor = successorNode;
        fingerTable[0].node = successorNode;

        Chord *successorChord = proxy->findChordNodeByID(successorNode.id);
        Node oldPredecessorOfSuccessor;
        if (successorChord)
            oldPredecessorOfSuccessor = successorChord->getPredecessor();
        if (!oldPredecessorOfSuccessor.isEmpty() && oldPredecessorOfSuccessor != self)
        {
            predecessor = oldPredecessorOfSuccessor;
        }
        else
        {
            predecessor = successorNode;
        }

        if (successorChord)
            successorChord->notifyPredecessor(self);

        if (!oldPredecessorOfSuccessor.isEmpty() && oldPredecessorOfSuccessor != self && oldPredecessorOfSuccessor != successorNode)
        {
            Chord *oldPredChord = proxy->findChordNodeByID(oldPredecessorOfSuccessor.id);
            if (oldPredChord)
                oldPredChord->setSuccessor(self);
        }

        initFingerTable();
        notifyRelevantNodes();
        redistributeResources();
        fixFingers();

        logger.info("节点 " + self.toString() + " 初始化完成");
    }
    catch (const exception &e)
    {
        logger.error("初始化失败: " + string(e.what()));
        initAsFirstNode();
    }
}

/**
 * @brief 初始化Chord环中的节点，使其成为第一个节点
 */
void Chord::initFingerTable()
{
    if (!proxy)
    {
        for (int i = 1; i < m; i++)
            fingerTable[i].node = successor;
        return;
    }

    fingerTable[0].node = successor;

    for (int i = 1; i < m; i++)
    {
        uint32_t start = fingerTable[i].startId;
        Node succ = proxy->findSuccessorFromAny(start);
        if (!succ.isEmpty())
            fingerTable[i].node = succ;
        else
            fingerTable[i].node = successor;
    }
}

/**
 * @brief 通知设置前驱节点为n
 * @param n 前驱节点
 */
void Chord::notifyPredecessor(Node n)
{
    if (n.isEmpty() || n == self)
        return;
    if (predecessor.isEmpty() || predecessor == self)
    {
        predecessor = n;
    }
    else if (isInOpenInterval(n.id, predecessor.id, self.id))
    {
        predecessor = n;
    }
}

/**
 * @brief 通知Chord环中所有相关节点关于当前节点的变化
 */
void Chord::notifyRelevantNodes()
{
    // 已经使用通知所有节点的方式了，这里随便怎么写更新节点的代码都无所谓了doge
    if (!proxy)
        return;
    vector<uint32_t> allIds = proxy->getAllSortedNodeIds();
    int n = allIds.size();
    if (n <= 1)
        return;

    int selfPos = -1;
    for (int i = 0; i < n; i++)
    {
        if (allIds[i] == self.id)
        {
            selfPos = i;
            break;
        }
    }
    if (selfPos == -1)
        return;

    int half = n / 2;
    for (int i = 1; i <= half; i++)
    {
        int idx = (selfPos - i + n) % n;
        uint32_t otherId = allIds[idx];
        Chord *chord = proxy->findChordNodeByID(otherId);
        if (chord)
            chord->updateFingerTable(self);
    }
}

/**
 * @brief 更新Chord环中的节点的finger table
 * @param newNode 新的节点
 */
void Chord::updateFingerTable(Node &newNode)
{
    if (newNode.isEmpty())
        return;
    for (int i = 0; i < m; i++)
    {
        uint32_t start = fingerTable[i].startId;
        Node current = fingerTable[i].node;

        bool shouldUpdate = false;
        if (newNode.id == start)
        {
            shouldUpdate = true;
        }
        else if (isInOpenInterval(newNode.id, start, current.id))
        {
            shouldUpdate = true;
        }

        if (shouldUpdate && newNode != current)
        {
            fingerTable[i].node = newNode;
            if (i == 0)
                successor = newNode;
        }
    }
}

/**
 * @brief 修复（重置）Chord环中的节点的finger table
 */
void Chord::fixFingers()
{
    if (!proxy)
        return;
    for (int i = 1; i < m; i++)
    {
        Node succ = proxy->findSuccessorFromAny(fingerTable[i].startId);
        if (!succ.isEmpty())
            fingerTable[i].node = succ;
        else if (!successor.isEmpty())
            fingerTable[i].node = successor;
    }
}

/**
 * @brief 加入Chord环中
 */
void Chord::joinRing()
{
    logger.info("joinRing: " + self.toString());
    if (!proxy)
        throw runtime_error("无代理");
    if (proxy->isRingEmpty())
    {
        initAsFirstNode();
    }
    else
    {
        Node bootstrap = proxy->getAnyNodeInRing();
        if (bootstrap.isEmpty())
            throw runtime_error("无引导节点");
        if (bootstrap == self)
        {
            vector<Node> all = proxy->getAllNodesInRing();
            for (auto &n : all)
                if (n != self)
                {
                    bootstrap = n;
                    break;
                }
            if (bootstrap == self)
            {
                initAsFirstNode();
                return;
            }
        }
        initWithBootstrapNode(bootstrap);
    }
}

/**
 * @brief 重新分配Chord环中的节点的资源
 */
void Chord::redistributeResources()
{
    if (!proxy || successor.isEmpty() || successor == self)
        return;
    if (predecessor.isEmpty() || predecessor == self)
        return;

    Chord *succChord = proxy->findChordNodeByID(successor.id);
    if (!succChord)
        return;

    auto succRes = succChord->getAllResources();
    vector<pair<uint32_t, string>> toTransfer;

    for (auto &res : succRes)
    {
        if (isInInterval(res.first, predecessor.id, self.id))
        {
            toTransfer.push_back(res);
        }
    }

    for (auto &res : toTransfer)
    {
        if (succChord->removeResourceDirectly(res.first))
        {
            resources[res.first] = res.second;
        }
    }
}

/**
 * @brief 离开Chord环，将资源转移给后继节点
 * @return 如果转移成功返回true，否则返回false
 */
bool Chord::transferResources()
{
    if (!proxy || resources.empty() || successor == self)
        return true;
    for (auto &res : resources)
    {
        proxy->transferResourceToNode(successor, res.second);
    }
    resources.clear();
    return true;
}

/**
 * @brief 离开Chord环
 * @return 如果离开成功返回true，否则返回false
 */
bool Chord::leaveRing()
{
    if (!proxy)
        return false;
    if (successor == self)
    {
        resources.clear();
        return true;
    }

    transferResources();

    if (!predecessor.isEmpty() && predecessor != self)
    {
        Chord *predChord = proxy->findChordNodeByID(predecessor.id);
        if (predChord)
            predChord->setSuccessor(successor);
    }
    if (!successor.isEmpty() && successor != self)
    {
        Chord *succChord = proxy->findChordNodeByID(successor.id);
        if (succChord)
            succChord->setPredecessor(predecessor);
    }

    predecessor = Node();
    successor = Node();
    resources.clear();
    logger.info(self.toString() + " 已离开");
    return true;
}

/**
 * @brief 处理节点离开
 * @param leftNode 离开的节点
 */
void Chord::handleNodeLeave(Node &leftNode)
{
    if (leftNode.isEmpty() || leftNode == self)
        return;

    if (successor == leftNode)
    {
        Node newSucc;

        Chord *leftChord = proxy->findChordNodeByID(leftNode.id);
        if (leftChord)
        {
            newSucc = leftChord->getSuccessor();
        }

        if (newSucc.isEmpty() || newSucc == leftNode)
        {
            vector<uint32_t> sortedIds = proxy->getAllSortedNodeIds();
            auto it = find(sortedIds.begin(), sortedIds.end(), leftNode.id);
            if (it != sortedIds.end())
            {
                ++it;
                if (it != sortedIds.end())
                {
                    Chord *chord = proxy->findChordNodeByID(*it);
                    if (chord)
                        newSucc = chord->getSelf();
                }
                else if (!sortedIds.empty())
                {
                    Chord *chord = proxy->findChordNodeByID(sortedIds[0]);
                    if (chord)
                        newSucc = chord->getSelf();
                }
            }
        }

        if (!newSucc.isEmpty() && newSucc != leftNode)
        {
            successor = newSucc;
            fingerTable[0].node = newSucc;
        }
        else
        {
            successor = self;
            fingerTable[0].node = self;
        }
    }

    if (predecessor == leftNode)
    {
        predecessor = Node();
    }

    for (int i = 1; i < m; i++)
    {
        if (fingerTable[i].node == leftNode)
        {
            Node newSucc = proxy->findSuccessorFromAny(fingerTable[i].startId);
            if (!newSucc.isEmpty() && newSucc != leftNode)
            {
                fingerTable[i].node = newSucc;
            }
            else
            {
                fingerTable[i].node = successor.isEmpty() ? self : successor;
            }
        }
    }

    fixFingers();
}

/**
 * @brief 添加资源到Chord环中
 * @param resource 资源内容
 * @return 如果添加成功返回true，否则返回false
 */
bool Chord::addResource(string resource)
{
    uint32_t rid = sha1_hash_to_uint32(resource) % ID_SPACE;
    if (resources.count(rid))
        return false;
    resources[rid] = resource;
    return true;
}

/**
 * @brief 直接添加资源到Chord环中
 * @param rid 资源ID
 * @param res 资源内容
 * @return 如果添加成功返回true，否则返回false
 */
bool Chord::addResourceDirectly(uint32_t rid, const string &res)
{
    logger.info("addResourceDirectly: " + self.toString() + " -> " + to_string(rid));
    resources[rid] = res;
    return true;
}

/**
 * @brief 获取Chord环中的所有资源
 * @return 资源ID到资源内容的映射
 */
map<uint32_t, string> Chord::getAllResources() const { return resources; }

/**
 * @brief 直接移除Chord环中的资源
 * @param rid 资源ID
 * @return 如果移除成功返回true，否则返回false
 */
bool Chord::removeResourceDirectly(uint32_t rid)
{
    logger.info("removeResourceDirectly: " + self.toString() + " -> " + to_string(rid));
    auto it = resources.find(rid);
    if (it != resources.end())
    {
        resources.erase(it);
        return true;
    }
    return false;
}

Node Chord::getSelf() const { return self; }
Node Chord::getSuccessor() const { return successor; }
Node Chord::getPredecessor() const { return predecessor; }
int Chord::getResourceCount() const { return resources.size(); }

void Chord::setPredecessor(const Node &n)
{
    logger.info("setPredecessor: " + self.toString() + " -> " + n.toString());
    predecessor = n;
}

void Chord::setSuccessor(const Node &n)
{
    logger.info("setSuccessor: " + self.toString() + " -> " + n.toString());
    successor = n;
    fingerTable[0].node = n;
}

void Chord::showNodeInfo() const
{
    cout << "\n========== " << self.toString() << " ==========" << endl;
    cout << "前驱: " << (predecessor.isEmpty() ? "无" : predecessor.toString()) << endl;
    cout << "后继: " << (successor.isEmpty() ? "无" : successor.toString()) << endl;
    cout << "资源 (" << resources.size() << " 个):" << endl;
    if (resources.empty())
        cout << "  无" << endl;
    else
    {
        for (const auto &res : resources)
        {
            cout << "  ID: " << res.first << " -> " << res.second << endl;
        }
    }
    cout << "Finger Table:" << endl;
    for (int i = 0; i < m; i++)
    {
        cout << "  [" << i << "] " << fingerTable[i].startId << " -> ";
        if (fingerTable[i].node.isEmpty())
            cout << "EMPTY";
        else
            cout << fingerTable[i].node.toString();
        cout << endl;
    }
    cout << "============================================" << endl;
}

// ==================== 新增 CLI 辅助方法 ====================

/**
 * @brief 加入Chord环中的节点
 * @param ip 节点IP
 * @return 如果加入成功返回true，否则返回false
 */
bool ChordRingManager::join(const std::string &ip)
{
    if (nodeExists(ip))
    {
        logger.warning("节点IP " + ip + " 已存在，无法重复添加");
        return false;
    }
    Node newNode(ip);
    Chord *chord = new Chord(newNode, &proxy);
    if (!join(newNode, chord))
    {
        delete chord;
        return false;
    }
    chord->joinRing();
    return true;
}

/**
 * @brief 移除Chord环中的节点
 * @param ip 节点IP
 * @return 如果移除成功返回true，否则返回false
 */
bool ChordRingManager::removeNodeByIP(const std::string &ip)
{
    Node node = getNodeByIP(ip);
    if (node.isEmpty())
    {
        logger.warning("节点IP " + ip + " 不存在，无法删除");
        return false;
    }
    return removeNode(node);
}

/**
 * @brief 获取Chord环中指定IP的节点
 * @param ip 节点IP
 * @return 如果节点存在返回节点，否则返回空节点
 */
Node ChordRingManager::getNodeByIP(const std::string &ip) const
{
    for (const auto &p : chordNodes)
    {
        if (p.second->getSelf().ip == ip)
            return p.second->getSelf();
    }
    return Node();
}

/**
 * @brief 检查Chord环中是否存在指定IP的节点
 * @param ip 节点IP
 * @return 如果节点存在返回true，否则返回false
 */
bool ChordRingManager::nodeExists(const std::string &ip) const
{
    return !getNodeByIP(ip).isEmpty();
}

/**
 * @brief 获取所有Chord环中的节点IP
 * @return Chord环中的所有节点IP
 */
std::vector<std::string> ChordRingManager::getAllNodeIPs() const
{
    std::vector<std::string> ips;
    for (const auto &p : chordNodes)
    {
        ips.push_back(p.second->getSelf().ip);
    }
    return ips;
}

/**
 * @brief 移除Chord环中的资源
 * @param resourceName 资源名称
 * @return 如果移除成功返回true，否则返回false
 */
bool ChordRingManager::removeResource(const std::string &resourceName)
{
    if (chordNodes.empty())
        return false;
    uint32_t rid = sha1_hash_to_uint32(resourceName) % ID_SPACE;
    Node anyNode = getAnyNode();
    if (anyNode.isEmpty())
        return false;
    Node responsible = findSuccessor(anyNode, rid);
    Chord *chord = findChordNode(responsible.id);
    if (!chord)
        return false;
    const auto &resources = chord->getAllResources();
    auto it = resources.find(rid);
    if (it == resources.end())
        return false;
    chord->removeResourceDirectly(rid);
    logger.info("资源 '" + resourceName + "' 从节点 " + responsible.toString() + " 移除");
    return true;
}

/**
 * @brief 获取所有Chord环中的资源名称
 * @return Chord环中的所有资源名称
 */
std::vector<std::string> ChordRingManager::getAllResourceNames() const
{
    std::vector<std::string> names;
    for (const auto &p : chordNodes)
    {
        for (const auto &res : p.second->getAllResources())
            names.push_back(res.second);
    }
    return names;

}
