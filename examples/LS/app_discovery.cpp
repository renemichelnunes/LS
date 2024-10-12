#include <app_discovery.hpp>

discovery_node::discovery_node(char * node_id, uint8_t hops, uint16_t gridX, uint16_t gridY)
{
    strcpy(this->gridLocalization.node_id, node_id);
    this->gridLocalization.hops = hops;
    this->gridLocalization.gridX = gridX;
    this->gridLocalization.gridY = gridY;
}

void discovery_app::show(lv_event_t *e)
{
    lv_obj_clear_flag(this->frm_discovery, LV_OBJ_FLAG_HIDDEN);
}

void discovery_app::hide(lv_event_t *e)
{
    lv_obj_add_flag(this->frm_discovery, LV_OBJ_FLAG_HIDDEN);
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

bool compareByHops(const grid_localization & a, const grid_localization & b){
    return a.hops < b.hops;
}

bool discovery_app::add(discovery_node node)
{
    for(discovery_node dn : this->list)
        if(strcmp(dn.gridLocalization.node_id, node.gridLocalization.node_id) == 0)
            return false;
    this->list.push_back(node);
    std::sort(this->list.begin(), this->list.end(), compareByHops);
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
