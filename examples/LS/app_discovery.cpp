#include <app_discovery.hpp>

discovery_node::discovery_node(char *node_id, uint8_t hops)
{
    strcpy(this->node_id, node_id);
    this->hops = hops;
}

bool discovery_app::exists(const char *node_id)
{
    for(discovery_node node : this->list){
        if(strcmp(node.node_id, node_id) == 0){
            return true;
        }
    }
    return false;
}

bool discovery_app::add(discovery_node node)
{
    for(discovery_node dn : this->list)
        if(strcmp(dn.node_id, node.node_id) == 0)
            return false;
    this->list.push_back(node);
    return true;
}

discovery_node * discovery_app::getNode(const char *node_id)
{
    for(uint8_t i = 0; i < this->list.size(); i++){
        if(strcmp(this->list[i].node_id, node_id) == 0){
            return &this->list[i];
        }
    }
    return nullptr;
}
