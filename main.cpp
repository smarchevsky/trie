#include <stdint.h>
#include <stdio.h>
#include <unordered_map>

// clang-format off
enum Op : int8_t { opUnknown = 0, parOpen, parClose, opPlus, opMinus, opMul, opDiv, opNegate, opSin, opCos, opTan, opCat, opCar };
// clang-format on

struct TrieNode {
    Op op = opUnknown;
    std::unordered_map<char, TrieNode*> children;
    void print(int offset) const
    {
        putchar('\n');
        offset++;
        for (const auto& [c, node] : children) {
            for (int i = 0; i < offset; i++)
                putchar(' ');
            printf("%c", c);

            node->print(offset);
        }
    }
};

class Trie {
    TrieNode root;
    void clear(TrieNode& n)
    {
        for (auto& c : n.children) {
            clear(*c.second);
            delete c.second;
        }
    }

public:
    ~Trie() { clear(root); };

    void insert(const char* word, Op op)
    {
        if (op == opUnknown)
            return;

        TrieNode* node = &root;
        while (char c = *word) {
            node = node->children[c] ? node->children[c] : (node->children[c] = new TrieNode());
            word++;
        }
        node->op = op;
    }

    void print()
    {
        root.print(0);
    }

    int match(const char* text, int start, Op& op) const
    {
        const TrieNode* node = &root;
        int len = 0;

        for (int i = start; text[i]; ++i) {
            auto it = node->children.find(text[i]);
            if (it == node->children.end())
                break;

            node = it->second;
            ++len;

            if (node->op == opUnknown) {
                op = node->op;
                return len;
            }
        }

        return 0;
    }
};

int main()
{
    Trie trie;
    trie.insert("sin", opSin);
    trie.insert("cos", opCos);
    trie.insert("-", opMinus);
    trie.insert("cat", opCat);
    trie.insert("car", opCat);
    trie.print();
    return 0;
}
