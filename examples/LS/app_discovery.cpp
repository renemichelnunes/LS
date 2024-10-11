#include <app_discovery.hpp>

discovery_node::discovery_node(char * node_id, uint8_t hops, uint16_t gridX, uint16_t gridY)
{
    strcpy(this->gridLocalization.node_id, node_id);
    this->gridLocalization.hops = hops;
    this->gridLocalization.gridX = gridX;
    this->gridLocalization.gridY = gridY;
}

bool discovery_app::exists(const char *node_id)
{
    for(discovery_node node : this->list){
        if(strcmp(node.gridLocalization.node_id, node_id) == 0){
            return true;
        }
    }
    return false;
}

bool discovery_app::add(discovery_node node)
{
    for(discovery_node dn : this->list)
        if(strcmp(dn.gridLocalization.node_id, node.gridLocalization.node_id) == 0)
            return false;
    this->list.push_back(node);
    return true;
}

discovery_node * discovery_app::getNode(const char *node_id)
{
    for(uint8_t i = 0; i < this->list.size(); i++){
        if(strcmp(this->list[i].gridLocalization.node_id, node_id) == 0){
            return &this->list[i];
        }
    }
    return nullptr;
}

uint32_t discovery_app::hopsTo(const char *node_id)
{
    for(discovery_node node : this->list)
        if(strcmp(node.gridLocalization.node_id, node_id) == 0)
            return node.gridLocalization.hops;
    return 0;
}
