#include <typeinfo>
#include "RMTree.h"
#include "DOCXToRM.h"

RMTree::RMTree()
{
    Tree[Root.node_ID] = Root;
};
    
void RMTree::AddBlock(RMBlock* block)
{
    //char MessageBuffer[1024];
    //sprintf_s(MessageBuffer, "<D> %s", typeid(*block).name());
    //DoLog(MessageBuffer);

    switch (block->Class)
    {
    case BlockClass::SceneTreeClass:
        AddNode((SceneTree*)block);
        break;
    case BlockClass::TreeNodeClass:
        UpdateNode((TreeNode*)block);
        break;
    case BlockClass::GroupItem:
        ReplaceNode((SceneGroupItem*)block);
        break;
    case BlockClass::Item:
        AddItem((RMSceneItemBlock*)block);
        break;
    case BlockClass::RootTextClass:
        //        if tree.root_text is not None :
        //            _logger.error(
        //                "Overwriting root text\n  Old: %s\n  New: %s",
        //                tree.root_text,
        //                b.value,
        //                )
        //            tree.root_text = b.value
        break;
    default:
        break;
    }


}

void RMTree::AddNode(SceneTree* block) {

    if (block->node_id == RM_CRDT_ID{0, 0}) {
        DoLog(typeid(*this).name(), "Blank Node", LOG_DEBUG);
    //    return;
    }

    if (Tree.contains(block->node_id)) {
//        throw std::range_error("Repeated Node");
        DoLog(typeid(*this).name(), "Repeated Node", LOG_DEBUG);
    }

    RMTreeGroup NewGroup(block->node_id);
    Tree[block->node_id] = NewGroup;

    sprintf_s(LogBuffer, LB_SIZE, "Adding Tree Node: Node ID (%d, %d) Parent ID(%d, %d)...",
        block->node_id.part1, block->node_id.part2, block->parent_id.part1, block->parent_id.part2
    );
    DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG_VERBOSE);

    //        # parent = self._node_ids[parent_id]
    //        # parent.children.add(item)
}

void RMTree::UpdateNode(TreeNode* block)
{
    if (!Tree.contains(block->node_id)) {
        DoLog(typeid(*this).name(), "Missing Node", LOG_DEBUG);
//        throw std::range_error("Missing Node");
    }

    RMTreeGroup node(Tree[block->node_id]);
    node.Update(block);
}

void RMTree::ReplaceNode(SceneGroupItem* block)
{
    //        # Add this entry to children of parent_id
    //        node_id = b.item.value   --- this is ID5
    //        if node_id not in tree :
    //    raise ValueError(
    //        "Node does not exist for SceneGroupItemBlock: %s" % node_id
    //    )
    //        item = replace(b.item, value = tree[node_id])
    //        tree.add_item(item, b.parent_id)
    if (!Tree.contains(block->item_id)) {
        DoLog(typeid(*this).name(), "Missing Node", LOG_DEBUG_VERBOSE);
        //        throw std::range_error("Missing Node");
    }
    AddItem(block);
//    RMTreeGroup node(Tree[block->node_id]);
}


void RMTree::AddItem(RMSceneItemBlock* block)
{
    if (!Tree.contains(block->parent_id)) {
        DoLog(typeid(*this).name(), "Missing Node", LOG_DEBUG_VERBOSE);
    }
    Tree[block->parent_id].AddChild(block);
}


// Iterator functions...
RMTree::Iterator::Iterator(RMTreeGroup * RMTG) {
    CurrentNode = RMTG;
    ChildIndex = 0;
}

// Prefix increment
RMTree::Iterator& RMTree::Iterator::operator++() 
{
    ChildIndex++;
    if (ChildIndex >= CurrentNode->Children.size()) {
        //Finished this nodes children, on to the next node
    }
    
    return *this; 
}

// Postfix increment
RMTree::Iterator RMTree::Iterator::operator++(int) 
{ 
    RMTree::Iterator tmp = *this; 
    ++(*this); 
    return tmp; 
}


RMTree::Iterator RMTree::begin() {
    return Iterator(&Root); 
}

RMTree::Iterator RMTree::end() 
{ 
    return NULL;
//    return Iterator(&Iterator::m_data[200]); 
} 











const RM_CRDT_ID Root_CRDTID = { 0, 1 };

RMTreeGroup::RMTreeGroup() {
    node_ID = Root_CRDTID;
}


RMTreeGroup::RMTreeGroup(RM_CRDT_ID Node_ID) {
	node_ID = Node_ID;
}

void RMTreeGroup::Update(TreeNode* block) {
    label = block->label;
    visible = block->visible;
    anchor_id = block->anchor_id;
    anchor_type = block->anchor_type;
    anchor_threshold = block->anchor_threshold;
    anchor_origin_x = block->anchor_origin_x;

    sprintf_s(LogBuffer, LB_SIZE, "Updating Tree Node: Node ID (%d, %d)...",
        block->node_id.part1, block->node_id.part2
    );
    DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG_VERBOSE);
}

void RMTreeGroup::AddChild(RMSceneItemBlock* block) {
    Children.push_back(block);
    sprintf_s(LogBuffer, LB_SIZE, "Adding child block (%s) to Tree Node: Node ID (%d, %d)...",
        typeid(*block).name(), block->item_id.part1, block->item_id.part2
    );
    DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG_VERBOSE);
}