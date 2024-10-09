#include <app_discovery.hpp>

discovery_node::discovery_node(char *node_id, uint8_t hops)
{
    strcpy(this->node_id, node_id);
    this->hops = hops;
}

bool discovery::exists(const char *node_id)
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

discovery_node * discovery::getNode(const char *node_id)
{
    for(uint8_t i = 0; i < this->list.size(); i++){
        if(strcmp(this->list[i].node_id, node_id) == 0){
            return &this->list[i];
        }
    }
    return nullptr;
}
