//
// Created by Haotian on 17/6/5.
//

#ifndef MINISQL_BPTREE_H
#define MINISQL_BPTREE_H

#include <iostream>
#include <vector>
#include <string>

using namespace std;

template<typename T>
class BPTreeNode {
public:
    BPTreeNode(int degree, bool isLeaf);

    ~BPTreeNode() {}

    bool search(const T &key, int &index) const;
    BPTreeNode *split(T &key);
    int add(const T &key);
    int add(const T &key, int offset);
    void removeAt(int index);
    bool isRoot() const { return parent == nullptr; }

    bool isLeaf;
    int degree, cnt;
    BPTreeNode *parent, *sibling;
    vector<T> keys;
    vector<int> keyOffset;
    vector<BPTreeNode> children;

private:
    bool binarySearch(const T &key, int &index) const;
};

BPTreeNode::BPTreeNode(int degree, bool isLeaf) : degree(degree), isLeaf(isLeaf), cnt(0), parent(nullptr),
                                                  sibling(nullptr) {
    children.resize(degree + 1);
    keys.resize(degree);
    keyOffset.resize(degree);
}

template<typename T>
bool BPTreeNode::search(const T &key, int &index) const {
    if (cnt == 0) {
        index = 0;
        return false;
    }
    if (keys[0] > key) {
        index = 0;
        return false;
    }
    if (keys[cnt - 1] < key) {
        index = cnt;
        return false;
    }
    return binarySearch(key, index);
}

template<typename T>
bool BPTreeNode::binarySearch(const T &key, int &index) const {
    int left = 0, right = cnt - 1, pos;
    while (left <= right) {
        pos = left + (right - left) / 2;
        if (keys[pos] < key) {
            left = pos + 1;
        } else {
            right = pos - 1;
        }
    }
    index = left;
    return keys[index] == key;
}

template<typename T>
BPTreeNode *BPTreeNode::split(T &key) {
    BPTreeNode* newNode = new BPTreeNode(degree, isLeaf);
    int minimal;

    if (isLeaf) {
        minimal = degree / 2;
        key = keys[minimal];
        for (int i=minimal; i<degree; i++) {
            newNode->keys[i - minimal] = keys[i];
            newNode->keyOffset[i - minimal] = keyOffset[i];
        }
        newNode->sibling = this->sibling;
        this->sibling = newNode;
    } else {
        minimal = (degree + 1) / 2;
        key = keys[minimal];
        for (int i=minimal; i<=degree; i++) {
            newNode->keys[i - minimal] = keys[i];
            newNode->children[i - minimal] = this->children[i];
            newNode->children[i - minimal]->parent = newNode;
            this->children[i] = nullptr;
        }
    }
    newNode->parent = this->parent;
    this->cnt = minimal;
    newNode->cnt = degree - minimal;
    return newNode;
}

template<typename T>
int BPTreeNode::add(const T &key) {
    int index;
    bool keyExists = search(key, index);
    if (keyExists) {
        cerr << "Key is not unique!" << endl;
        exit(10);
    }
    for (int i=cnt; i>index; i--) {
        keys[i] = keys[i-1];
        children[i+1] = keyOffset[i];
    }
    keys[index] = key;
    children[index + 1] = nullptr;
    cnt++;
    return index;
}

template<typename T>
int BPTreeNode::add(const T &key, int offset) {
    int index;
    bool keyExists = search(key, index);
    if (keyExists) {
        cerr << "Key is not unique!" << endl;
        exit(10);
    }
    for (int i=cnt; i>index; i--) {
        keys[i] = keys[i-1];
        keyOffset[i] = keyOffset[i-1];
    }
    keys[index] = key;
    keyOffset[index] = offset;
    cnt++;
    return index;
}

void BPTreeNode::removeAt(int index) {
    for (int i=index; i<cnt-1; i++) {
        keys[i] = keys[i+1];
    }
    if (isLeaf) {
        for (int i=index; i<cnt-1; i++) {
            keyOffset[i] = keyOffset[i+1];
        }
    } else {
        for (int i=index + 1; i<cnt; i++) {
            children[i] = children[i+1];
        }
    }
    cnt--;
}

template<typename T>
class BPTree {
public:
    BPTree(string fileName, int sizeofKey, int degree);
    ~BPTree();

    int find(const T &key);
    bool insert(const T &key, int offset);
    bool remove(const T &key);

private:
    typedef BPTreeNode<T>* TreeNode;
    struct NodeSearchParse {
        int index;
        TreeNode node;
    };
    string fileName;
    TreeNode root, head;
    int sizeofKey, level, keyCount, nodeCount, degree;

    void initBPTree();
    bool findKeyFromNode(TreeNode node, const T &key, NodeSearchParse &res);
    void cascadeInsert(TreeNode node);
    bool cascadeDelete(TreeNode node);
};

template<typename T>
BPTree::BPTree(string fileName, int sizeofKey, int degree) : fileName(fileName), sizeofKey(sizeofKey), degree(degree), keyCount(0), nodeCount(0), level(0), root(
        nullptr), head(nullptr) {
    initBPTree();
}

BPTree::~BPTree() {

}

template<typename T>
void BPTree::initBPTree() {
    root = new BPTreeNode<T>(degree, true);
    keyCount = 0;
    level = 1;
    nodeCount = 1;
    head = root;
}

template<typename T>
bool BPTree::findKeyFromNode(BPTree::TreeNode node, const T &key, NodeSearchParse &res) {
    int index;
    if (node->search(key, index)) {
        if (node->isLeaf) {
            res.index = index;
        } else {
            node = node->children[index + 1];
            while (!node->isLeaf) { node = node->children[0]; }
            res.index= 0;
        }
        res.node = node;
        return true;
    } else {
        if (node->isLeaf) {
            res.node = node;
            res.index = index;
            return false;
        } else {
            return findKeyFromNode(node->children[index], key, res);
        }
    }
}

template<typename T>
int BPTree::find(const T &key) {
    NodeSearchParse res;
    if (!root) { return -1; }
    if (findKeyFromNode(root, key, res)) { return res.node->keyOffset[res.index]; }
    else { return -1; }
}

template<typename T>
bool BPTree::insert(const T &key, int offset) {
    NodeSearchParse res;
    if (!root) { initBPTree(); }
    if (findKeyFromNode(root, key, res)) {
        cerr << "Insert duplicate key!" << endl;
        return false;
    }
    res.node->add(key, offset);
    if (res.node->cnt == degree) {
        cascadeInsert(res.node);
    }
    keyCount++;
    return true;
}

template<typename T>
void BPTree::cascadeInsert(BPTree::TreeNode node) {
    T key;
    TreeNode sibling = node->split(key);
    nodeCount++;

    if (node->isRoot()) {
        TreeNode root = new BPTreeNode<T>(degree, false);
        level++;
        nodeCount++;
        this->root = root;
        node->parent = root;
        sibling->parent = root;
        root->add(key);
        root->children[0] = node;
        root->children[1] = sibling;
    } else {
        TreeNode parent = node->parent;
        int index = parent->add(key);

        parent->children[index + 1] = sibling;
        sibling->parent = parent;
        if (parent->cnt == degree) {
            cascadeInsert(parent);
        }
    }
}

template<typename T>
bool BPTree::remove(const T &key) {
    NodeSearchParse res;
    if (!root) {
        cerr << "Dequeuing empty BPTree!" << endl;
        return false;
    }
    if (!findKeyFromNode(root, key, res)) {
        cerr << "Key not found!" << endl;
        return false;
    }
    if (res.node->isRoot()) {
        res.node->removeAt(res.index);
        keyCount--;
        return cascadeDelete(res.node);
    } else {
        if (res.index == 0 && head != res.node) {
            // cascadingly update parent node
            int index;
            TreeNode currentParent = res.node->parent;
            bool keyFound = currentParent->search(key, index);
            while (!keyFound) {
                if (!currentParent->parent) { break; }
                currentParent = currentParent->parent;
                keyFound = currentParent->search(key, index);
            }
            currentParent->keys[index] = res.node->keys[1];
            res.node->removeAt(res.index);
            keyCount--;
            return cascadeDelete(res.node);
        } else {
            res.node->removeAt(res.index);
            keyCount--;
            return cascadeDelete(res.node);
        }
    }
}

template<typename T>
bool BPTree::cascadeDelete(BPTree::TreeNode node) {
    int minimal = degree / 2, minimalBranch = (degree + 1) / 2;
    if ((node->isLeaf && node->cnt >= minimal) // leaf node
        || (node->isRoot() && node->cnt) // root node
        || (!node->isLeaf && !node->isRoot() && node->cnt >= minimalBranch) // branch node
            ) {
        return true; // no need to update
    }

    if (node->isRoot()) {
        if (node->isLeaf) {
            // tree completely removed
            root = nullptr;
            head = nullptr;
        } else {
            // reduce level by one
            root = node->children[0];
            root->parent = nullptr;
        }
        delete node;
        nodeCount--;
        level--;
        return true;
    }
    TreeNode currentParent = node->parent, sibling;

    if (node->isLeaf) {
        int index;
        currentParent->search(node->keys[0], index);
        if (currentParent->children[0] != node && currentParent->cnt == index + 1) {
            // sibling
        }
    }

    return false;
}

#endif //MINISQL_BPTREE_H
