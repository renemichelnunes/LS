#include <app_discovery.hpp>

discovery_node::discovery_node(char *node_id, uint8_t hops)
{
    strcpy(this->node_id, node_id);
    this->hops = hops;
}

bool discovery::exists(char *node_id)
{
    for(discovery_node node : this->list){
        if(strcmp(node.node_id, node_id) == 0){
            return true;
        }
    }
    return false;
}

void discovery::add(discovery_node node)
{
    this->list.push_back(node);
}

discovery_node *discovery::getNode(char *node_id)
{
    for(discovery_node node : this->list){
        if(strcmp(node.node_id, node_id) == 0){
            return &node;
        }
    }
    return nullptr;
}
