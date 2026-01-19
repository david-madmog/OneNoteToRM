#pragma once
#include <map>
#include <vector>
#include "RMBlockTypes.h"

class RMTreeGroup
{
public:
    RMTreeGroup();
    RMTreeGroup(RM_CRDT_ID node_ID);
    void Update(TreeNode* block);
    void AddChild(RMSceneItemBlock* block);

    RM_CRDT_ID node_ID;
    std::vector<RMSceneItemBlock*> Children;
private:
    //"""A Group represents a group of nested items.

    //    Groups are used to represent layers.
    //    node_id is the id that this sub - tree is stored as a "SceneTreeBlock".
    //    children is a sequence of other SceneItems.
    //    `anchor_id` refers to a text character which provides the anchor y - position
    //    for this group.There are two values that seem to be special :
    //-`0xfffffffffffe` seems to be used for lines right at the top of the page ?
    //    -`0xffffffffffff` seems to be used for lines right at the bottom of the page ?

    RM_LWW_String label;
    RM_LWW_Bool visible;
    RM_LWW_ID anchor_id;
    RM_LWW_Byte anchor_type;
    RM_LWW_Float anchor_threshold;
    RM_LWW_Float anchor_origin_x;

};

class RMTree
{
public:
    RMTree();
    void AddBlock(RMBlock* block);

    struct Iterator
    {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = RMSceneItemBlock;
        using pointer = RMSceneItemBlock*;  // or also value_type*
        using reference = RMSceneItemBlock&;  // or also value_type&

        Iterator(RMTreeGroup* RMTG);

        reference operator*() const { return *(CurrentNode->Children[ChildIndex]); }
        pointer operator->() { return CurrentNode->Children[ChildIndex]; }

        // Prefix increment
        Iterator& operator++();

        // Postfix increment
        Iterator operator++(int);

        friend bool operator== (const Iterator& a, const Iterator& b) { return a.CurrentNode->Children[a.ChildIndex] == b.CurrentNode->Children[b.ChildIndex]; };
        friend bool operator!= (const Iterator& a, const Iterator& b) { return a.CurrentNode->Children[a.ChildIndex] != b.CurrentNode->Children[b.ChildIndex]; };


    private:
        RMTreeGroup * CurrentNode;
        int ChildIndex;
    };

    Iterator begin();
    Iterator end(); // 200 is out of bounds
    std::map<RM_CRDT_ID, RMTreeGroup> Tree;

private:
    RMTreeGroup Root ;

    void AddNode(SceneTree* block);
    void UpdateNode(TreeNode* block);
    void ReplaceNode(SceneGroupItem* block);
    void AddItem(RMSceneItemBlock* block);

};

