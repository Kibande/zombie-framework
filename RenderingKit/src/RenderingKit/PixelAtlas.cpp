
#include <RenderingKit/RenderingKitUtility.hpp>

namespace RenderingKit
{
    PixelAtlas::Node::~Node()
    {
        if (children[0] != nullptr)
        {
            delete children[0];
            delete children[1];
        }
    }

    PixelAtlas::PixelAtlas(Int2 size, Int2 border, Int2 alignBits, Int2 minCell)
            : border(border), minCell(minCell)
    {
        root = new Node;
        root->flags = 0;
        root->pos = Int2();
        root->size = size;
        root->children[0] = nullptr;
        root->children[1] = nullptr;

        align.x = (1 << alignBits.x) - 1;
        align.y = (1 << alignBits.y) - 1;
    }

    PixelAtlas::~PixelAtlas()
    {
        delete root;
    }

    bool PixelAtlas::Alloc(Int2 size, Int2& pos_out)
    {
        size += 2 * border;

        size.x = (glm::max(size.x, minCell.x) + align.x) & ~align.x;
        size.y = (glm::max(size.y, minCell.y) + align.y) & ~align.y;

        return Alloc(root, size, pos_out);
    }

    bool PixelAtlas::Alloc(Node* node, Int2 size, Int2& pos_out)
    {
        if ((node->flags & Node::FULLY_OCCUPIED) || size.x > node->size.x || size.y > node->size.y)
            return false;

        if (node->children[0] != nullptr)
            return Alloc(node->children[0], size, pos_out) || Alloc(node->children[1], size, pos_out);

        const int wasteHor = size.x * (node->size.y - size.y);
        const int wasteVert = size.y * (node->size.x - size.x);

        Node* allocated = (wasteHor < wasteVert) ? HorSplit(node, size) : VertSplit(node, size);
        pos_out = allocated->pos + border;
        return true;
    }

    PixelAtlas::Node* PixelAtlas::HorSplit(Node* node, Int2 size)
    {
        Node* left = new Node;
        left->flags = 0;
        left->pos = node->pos;
        left->size = Int2(size.x, node->size.y);
        left->children[0] = nullptr;
        left->children[1] = nullptr;

        Node* right = new Node;
        right->flags = 0;
        right->pos = Int2(node->pos.x + size.x, node->pos.y);
        right->size = Int2(node->size.x - size.x, node->size.y);
        right->children[0] = nullptr;
        right->children[1] = nullptr;

        node->children[0] = left;
        node->children[1] = right;

        if (left->size.y - size.y >= minCell.y)
            return VertSplit(left, size);
        else
        {
            left->flags |= Node::FULLY_OCCUPIED;
            return left;
        }
    }

    PixelAtlas::Node* PixelAtlas::VertSplit(Node* node, Int2 size)
    {
        Node* top = new Node;
        top->flags = 0;
        top->pos = node->pos;
        top->size = Int2(node->size.x, size.y);
        top->children[0] = nullptr;
        top->children[1] = nullptr;

        Node* bottom = new Node;
        bottom->flags = 0;
        bottom->pos = Int2(node->pos.x, node->pos.y + size.y);
        bottom->size = Int2(node->size.x, node->size.y - size.y);
        bottom->children[0] = nullptr;
        bottom->children[1] = nullptr;

        node->children[0] = top;
        node->children[1] = bottom;

        if (top->size.x - size.x >= minCell.x)
            return HorSplit(top, size);
        else
        {
            top->flags |= Node::FULLY_OCCUPIED;
            return top;
        }
    }
}
