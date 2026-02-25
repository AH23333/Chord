#include "node.h"
#include "config.h"
#include "SHA_1.h"
#include <string>

using namespace std;

Node::Node() : id(0), ip("") {}

Node::Node(string ip) : ip(ip)
{
    uint32_t hash = sha1_hash_to_uint32(ip);
    this->id = hash % ID_SPACE;
}

bool Node::operator==(const Node &other) const { return id == other.id; }
bool Node::operator!=(const Node &other) const { return !(*this == other); }
bool Node::operator<(const Node &other) const { return id < other.id; }
bool Node::isEmpty() const { return ip.empty() && id == 0; }
string Node::toString() const { return "Node(ID: " + to_string(id) + " ,IP: " + ip + " )"; }