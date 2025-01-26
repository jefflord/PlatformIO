#ifndef NODE_CODE_H // Header guard
#define NODE_CODE_H

#include <Arduino.h>
#include <chrono>
#include <iostream>
#include <vector>
#include <string>
#include <stdint.h>
#include <algorithm>
#include <functional>
#include <cstring> // Include for memcpy
#include <ThreadSafeSerial.h>

extern ThreadSafeSerial safeSerial;

struct Node
{
    std::string name;
    uint8_t macAddress[6];
    int status;
    int feetDeep = 0; // 0 = good
    int lostTokenReminder = 0;
    long long lastWorkTime;

    Node(const std::string &name, const uint8_t *mac, int status, long long timestamp) : name(name), status(status), lastWorkTime(timestamp)
    {
        memcpy(macAddress, mac, 6);
    }

    char *macAddressAsString()
    {
        char *macString = new char[18];
        sprintf(macString, "%02X:%02X:%02X:%02X:%02X:%02X", this->macAddress[0], this->macAddress[1], this->macAddress[2], this->macAddress[3], this->macAddress[4], this->macAddress[5]);
        return macString;
    }
};

class NodeList
{
private:
    std::vector<Node *> nodes_;

    static bool compareNodesByMac(const Node *a, const Node *b)
    {
        return memcmp(a->macAddress, b->macAddress, 6) < 0;
    }

public:
    void AddNode(Node *node)
    {
        nodes_.push_back(node);
    }

    Node *SetNextAsReady(Node *currentNode)
    {
        //currentNode->status = 0;
        
        if (nodes_.empty())
        {
            return nullptr; // Nothing to do if the list is empty
        }

        // Sort the nodes by MAC address
        std::sort(nodes_.begin(), nodes_.end(), compareNodesByMac);

        if (nodes_.size() == 1)
        {
            // Only one node, mark it as ready
            nodes_[0]->status = 1;
            return nodes_[0];
        }

        // Find the current node's position
        auto it = std::find(nodes_.begin(), nodes_.end(), currentNode);
        if (it == nodes_.end())
        {
            return nullptr; // Current node not found
        }

        // Find the next node (wrap around if necessary)
        auto nextIt = it + 1;
        if (nextIt == nodes_.end())
        {
            nextIt = nodes_.begin(); // Wrap around to the beginning
        }

        // Mark the next node as ready
        (*nextIt)->status = 1;

        // Reset the status of all other nodes (except the new ready one).
        for (Node *node : nodes_)
        {
            if (node != *nextIt)
            {
                node->status = 0;
            }
        }

        return *nextIt;
    }

    void RemoveNodes(std::function<bool(Node *)> predicate)
    {
        // Use an iterator to erase elements safely
        for (auto it = nodes_.begin(); it != nodes_.end();)
        {
            if (predicate(*it))
            {
                delete *it;            // Delete the node
                it = nodes_.erase(it); // Erase returns the next valid iterator
            }
            else
            {
                ++it;
            }
        }
    }

    bool SetStatusByMac(const uint8_t *mac, int status)
    {
        for (Node *node : nodes_)
        {
            if (memcmp(node->macAddress, mac, 6) == 0)
            {
                if (status != -1)
                {
                    // only change this to the new status if it's not -1, -1 is just an update
                    node->status = status;
                }

                node->feetDeep = 0;
                return true; // Found and updated
            }
        }
        return false; // Not found
    }

    void FreeTokenEverywhere()
    {
        for (Node *node : nodes_)
        {
            node->status = 0;
        }
    }

    Node *GetByMac(const uint8_t *mac)
    {
        for (Node *node : nodes_)
        {
            if (memcmp(node->macAddress, mac, 6) == 0)
            {
                return node; // Found and updated
            }
        }
        return nullptr; // Not found
    }

    void DeleteDeadNodes(Node *node, int howDeepIsDead)
    {
        // Use an iterator to erase elements safely
        for (auto it = nodes_.begin(); it != nodes_.end();)
        {
            if ((*it)->feetDeep >= howDeepIsDead && node != *it)
            {
                safeSerial.printf("Node %s is dead, feetDeep: %d\r\n", (*it)->macAddressAsString(), (*it)->feetDeep);
                delete *it;            // Delete the node
                it = nodes_.erase(it); // Erase returns the next valid iterator
            }
            else
            {
                ++it;
            }
        }
    }

    std::vector<Node *> GetAllNodes() const
    {
        return nodes_; // Creates a shallow copy of the vector
    }

    Node *GetNextReady(int status)
    {
        for (Node *node : nodes_)
        {
            if (node->status == status)
            {
                return node;
            }
        }

        auto node = nodes_[0]; // If no node is ready, return the first one
        node->status = 1;
        return node;
    }

    bool Contains(Node *node)
    {
        return std::any_of(nodes_.begin(), nodes_.end(), [node](Node *n)
                           { return n == node; });
    }

    ~NodeList()
    {
        for (Node *node : nodes_)
        {
            delete node;
        }
    }
};

#endif