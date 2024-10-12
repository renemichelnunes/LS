#include <app_discovery.hpp>

discovery_node::discovery_node(char * node_id, uint8_t hops, uint16_t gridX, uint16_t gridY)
{
    strcpy(this->gridLocalization.node_id, node_id);
    this->gridLocalization.hops = hops;
    this->gridLocalization.gridX = gridX;
    this->gridLocalization.gridY = gridY;
}

void show(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * frm = (lv_obj_t*)lv_event_get_target(e);
    if(frm)
        lv_obj_clear_flag(frm, LV_OBJ_FLAG_HIDDEN);
}

void hide(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * frm = (lv_obj_t*)lv_event_get_target(e);
    if(frm)
        lv_obj_add_flag(frm, LV_OBJ_FLAG_HIDDEN);
}

discovery_app::discovery_app(lv_obj_t * parent)
{
    this->parent = parent;
    this->initUI();

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

bool compareByHops(const discovery_node & a, const discovery_node & b){
    return a.gridLocalization.hops < b.gridLocalization.hops;
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

void discovery_app::initUI()
{
    // Main form
    this->frm_discovery = lv_obj_create(this->parent);
    lv_obj_set_size(this->frm_discovery, 300, 200);
    lv_obj_add_flag(this->frm_discovery, LV_OBJ_FLAG_HIDDEN);

    // Title button
    this->frm_discovery_btn_title = lv_btn_create(this->frm_discovery);
    lv_obj_set_size(this->frm_discovery_btn_title, 200, 20);
    lv_obj_align(this->frm_discovery_btn_title, LV_ALIGN_TOP_LEFT, -15, -15);

    this->frm_discovery_btn_title_lbl = lv_label_create(this->frm_discovery_btn_title);
    lv_label_set_text(this->frm_discovery_btn_title_lbl, "Discovered nodes");
    lv_obj_set_align(this->frm_discovery_btn_title_lbl, LV_ALIGN_LEFT_MID);

    // Close button
    this->frm_discovery_btn_back = lv_btn_create(this->frm_discovery);
    lv_obj_set_size(this->frm_discovery_btn_back, 50, 20);
    lv_obj_align(this->frm_discovery_btn_back, LV_ALIGN_TOP_RIGHT, 15, -15);
    //lv_obj_set_pos(btn_contacts, 10, -10);
    this->frm_discovery_btn_back_lbl = lv_label_create(this->frm_discovery_btn_back);
    lv_label_set_text(this->frm_discovery_btn_back_lbl, "Back");
    lv_obj_set_align(this->frm_discovery_btn_back_lbl, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(this->frm_discovery_btn_back_lbl, hide, LV_EVENT_SHORT_CLICKED, this->frm_discovery);

    // Add list object
    this->frm_discovery_nodeList = lv_list_create(this->frm_discovery);
    lv_obj_set_size(this->frm_discovery_nodeList, LV_HOR_RES, 220);
    lv_obj_align(this->frm_discovery_nodeList, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_border_opa(this->frm_discovery_nodeList, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(this->frm_discovery_nodeList, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void node_info(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_target(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        Serial.println("Node info");
    }
}

void discovery_app::updateNodeList()
{
    lv_obj_t * btn = NULL;
    char msg[100] = {'\0'};
    if(this->list.size() > 0){
        lv_obj_clean(this->frm_discovery_nodeList);
        for(discovery_node node : this->list){
            sprintf(msg, "%s is %i hops from you", node.gridLocalization.node_id, this->hopsTo(node.gridLocalization.node_id));
            btn = lv_list_add_btn(this->frm_discovery_nodeList, NULL, msg);
            lv_obj_add_event_cb(btn, node_info, LV_EVENT_SHORT_CLICKED, btn);
        }
    }
}

lv_obj_t *discovery_app::createNodeListObj(lv_obj_t * btn, const char *node_id, uint32_t hops)
{
    if(this->frm_discovery_nodeList){

    }
    return nullptr;
}

void discovery_app::showUI()
{
    if(this->frm_discovery){
        lv_obj_clear_flag(this->frm_discovery, LV_OBJ_FLAG_HIDDEN);
    }
}

void discovery_app::hideUI()
{
    if(this->frm_discovery){
        lv_obj_add_flag(this->frm_discovery, LV_OBJ_FLAG_HIDDEN);
    }
}
