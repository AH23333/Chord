#ifndef NODE_H
#define NODE_H

#include <string>
#include <cstdint>

struct Node
{
    uint32_t id;
    std::string ip;

    Node();
    explicit Node(std::string ip);
    bool operator==(const Node &other) const;
    bool operator!=(const Node &other) const;
    bool operator<(const Node &other) const;
    bool isEmpty() const;
    std::string toString() const;
};

struct FingerEntry
{
    uint32_t startId;
    Node node;
};

#endif // NODE_H