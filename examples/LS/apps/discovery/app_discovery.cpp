#include <apps/discovery/app_discovery.hpp>
#include "app_discovery.hpp"

TaskHandle_t task_update_nodes_list = NULL;
TaskHandle_t task_update_graph = NULL;

discovery_node::discovery_node(uint8_t hops, disc_node dn)
{
    this->node = dn;
    this->hops = hops;
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
    lv_obj_t * frm = (lv_obj_t*)lv_event_get_user_data(e);
    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm){
            if(task_update_nodes_list != NULL){
                vTaskDelete(task_update_nodes_list);
                task_update_nodes_list = NULL;
                Serial.println(F("task_update_nodes_list deleted"));
            }
            lv_obj_add_flag(frm, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

discovery_app::discovery_app(pthread_mutex_t * lvgl_mutex)
{
    this->new_node = false;
    this->lvgl_mutex = lvgl_mutex;
}

bool discovery_app::exists(const char *node_id)
{
    for(discovery_node dnode : this->list){
        if(strcmp(dnode.node.id, node_id) == 0){
            return true;
        }
    }
    return false;
}

bool compareByHops(const discovery_node & a, const discovery_node & b){
    return a.hops < b.hops;
}

bool discovery_app::add(discovery_node dnode)
{
    for(discovery_node dn : this->list)
        if(strcmp(dn.node.id, dnode.node.id) == 0)
            return false;
    this->list.push_back(dnode);
    std::sort(this->list.begin(), this->list.end(), compareByHops);
    this->new_node = true;
    return true;
}

discovery_node * discovery_app::getNode(const char *node_id)
{
    for(uint8_t i = 0; i < this->list.size(); i++){
        if(strcmp(this->list[i].node.id, node_id) == 0){
            return &this->list[i];
        }
    }
    return nullptr;
}

uint16_t discovery_app::hopsTo(const char *node_id)
{
    for(discovery_node dnode : this->list)
        if(strcmp(dnode.node.id, node_id) == 0)
            return dnode.hops;
    return 0;
}

void update_list(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    discovery_app * dis = (discovery_app*)lv_event_get_user_data(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(dis != NULL){
            dis->updateNodeList();
        }
        else
            Serial.println(F("dis is NULL"));
    }
}

void node_info(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_user_data(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        Serial.println(F("Node info"));
    }
}

void discovery_app::updateNodeList()
{
    lv_obj_t * btn = NULL;
    char msg[100] = {'\0'};
    if(this->new_node){
        if(this->list.size() > 0){
            lv_obj_clean(this->frm_discovery_nodeList);
            for(discovery_node dnode : this->list){
                sprintf(msg, "%s is at %i hop(s) from you", dnode.node.id, this->hopsTo(dnode.node.id));
                Serial.println(msg);
                btn = lv_list_add_btn(this->frm_discovery_nodeList, NULL, msg);
                lv_obj_add_event_cb(btn, node_info, LV_EVENT_SHORT_CLICKED, btn);
                this->new_node = false;
            }
        }
    }
    else
        Serial.println(F("No new node found so far"));
}

lv_obj_t *discovery_app::createNodeListObj(lv_obj_t * btn, const char *node_id, uint16_t hops)
{
    if(this->frm_discovery_nodeList){

    }
    return nullptr;
}

void up_nodes_list_task(void * p){
    discovery_app * dis = (discovery_app*)p;
    while(true){
        dis->updateNodeList();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void discovery_app::showUI()
{
    if(this->frm_discovery){
        lv_obj_clear_flag(this->frm_discovery, LV_OBJ_FLAG_HIDDEN);
        //this->updateNodeList();
        if(task_update_nodes_list == NULL){
            xTaskCreatePinnedToCore(up_nodes_list_task, (const char*)F("up_nodes_list"), 11000, this, 1, &task_update_nodes_list, 1);
            Serial.println(F("task_update_nodes_list launched"));
        }
    }
}

void discovery_app::hideUI()
{
    if(this->frm_discovery){
        if(task_update_nodes_list != NULL){
            vTaskDelete(task_update_nodes_list);
            task_update_nodes_list = NULL;
            Serial.println(F("task_update_nodes_list deleted"));
        }
        lv_obj_add_flag(this->frm_discovery, LV_OBJ_FLAG_HIDDEN);
    }
}

bool discovery_app::init_adj_matrix(){
    uint16_t num_nodes = this->list_demo.size();
    this->adjacency_matrix = new bool*[num_nodes];
    if(!this->adjacency_matrix){
        Serial.printf((const char*)F("Fail to alocate the matrix\n"));
    }
    for(uint16_t i = 0; i < num_nodes; i++){
        this->adjacency_matrix[i] = new bool[num_nodes]();
    }
    return true;
}

bool discovery_app::clear_adj_matrix(){
    uint16_t num_nodes = this->list_demo.size();
    for(uint16_t i = 0; i < num_nodes; i++){
        delete[] this->adjacency_matrix[i];
    }
    delete[] this->adjacency_matrix;
    
    return false;
}

void discovery_app::calculate_adj_matrix(){
    uint16_t num_nodes = this->list_demo.size();
    try{
        for(uint16_t i = 0; i < num_nodes; i++){
            for(char * neighbor : this->list_demo[i].node.neighbors){
                for(uint16_t j = 0; j < num_nodes; j++){
                    if(strcmp(neighbor, this->list_demo[j].node.id) == 0){
                        this->adjacency_matrix[i][j] = true;
                        break;
                    }
                }
            }
        }
        
        for(uint16_t i = 0; i < num_nodes; i++){
            for(uint16_t j = 0; j < num_nodes; j++){
                Serial.printf("%d ", this->adjacency_matrix[i][j]);
            }
            Serial.printf("\n");
        }
    }
    catch(std::exception &e){
        Serial.printf("%s\n", e.what());
    }
}

bool discovery_app::init_positions(){
    try{
        uint16_t num_nodes = this->list_demo.size();
        this->nodes.clear();
        for(uint16_t i = 0; i < num_nodes; i++){
            Node n;
            strcpy(n.id, this->list_demo[i].node.id);
            n.x = 20 + rand() % 200;
            n.y = 10 + rand() % 200;
            nodes.push_back(n);
        }
        return true;
    }
    catch(std::exception &e){
        Serial.printf("%s\n", e.what());
    }
    return false;
}

void discovery_app::calculate_repulsion(double * dx, double * dy, uint16_t i, uint16_t j){
    double dist_x = this->nodes[i].x - this->nodes[j].x;
    double dist_y = this->nodes[i].y - this->nodes[j].y;
    double distance = (double)sqrt(dist_x * dist_x + dist_y * dist_y) + 1e-6; // Evita divisão por zero

    double force = REPULSION_FORCE / (distance * distance);
    *dx += force * (dist_x / distance);
    *dy += force * (dist_y / distance);
}

void discovery_app::calculate_attraction(double *dx, double *dy, int i, int j) {
    /*
    if(this->adjacency_matrix == NULL)
        return;
        */
    if (this->adjacency_matrix[i][j]) {
        double dist_x = this->nodes[j].x - this->nodes[i].x;
        double dist_y = this->nodes[j].y - this->nodes[i].y;
        double distance = (double)sqrt(dist_x * dist_x + dist_y * dist_y) + 1e-6; // Evita divisão por zero

        double force = distance * (double)ATTRACTION_FORCE;
        *dx += force * (dist_x / distance);
        *dy += force * (dist_y / distance);
    }
}

void discovery_app::update_positions() {
    for (int i = 0; i < this->list_demo.size(); i++) {
        double dx = 0, dy = 0;

        // Calculates the repulsion forces
        for (int j = 0; j < this->list_demo.size(); j++) {
            if (i != j) {
                calculate_repulsion(&dx, &dy, i, j);
            }
        }

        // Calculates the atraction forces
        for (int j = 0; j < this->list_demo.size(); j++) {
            if (i != j) {
                calculate_attraction(&dx, &dy, i, j);
            }
        }

        // Update nodes positions
        nodes[i].x += dx * 5; // Multiplier to adjust the speed
        nodes[i].y += dy * 5;
    }
}

void show_graph(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    discovery_app * app = (discovery_app*) lv_event_get_user_data(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        app->show_graph_ui();
    }
}

void hide_graph(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * frm = (lv_obj_t*)lv_event_get_user_data(e);
    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm){
            if(task_update_graph){
                vTaskDelete(task_update_graph);
                task_update_graph = NULL;
                Serial.printf((const char*)F("task_update_graph deleted\n"));
            }
            lv_obj_add_flag(frm, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void update_graph_task(void * p){
    discovery_app * dis = (discovery_app*)p;
    while(true){
        dis->update_positions();
        dis->draw_graph();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void discovery_app::show_graph_ui(){
    if(this->frm_graph_main){
        lv_obj_clear_flag(this->frm_graph_main, LV_OBJ_FLAG_HIDDEN);
        if(!task_update_graph){
            xTaskCreatePinnedToCore(update_graph_task, (const char*)F("update_graph"), 11000, this, 1, &task_update_graph, 1);
            Serial.printf((const char*)F("task_update_graph launched\n"));
        }
    }
}

void discovery_app::hide_graph_ui(){
    lv_obj_clear_flag(this->frm_graph_main, LV_OBJ_FLAG_HIDDEN);
}

void discovery_app::draw_graph() {
    if(adjacency_matrix == NULL)
        return;
    // Desenha os nós
    pthread_mutex_lock(this->lvgl_mutex);
    lv_obj_clean(this->frm_graph_frame);
    pthread_mutex_unlock(this->lvgl_mutex);

    for (int i = 0; i < list_demo.size(); i++) {
        pthread_mutex_lock(this->lvgl_mutex);
        lv_obj_t *node = lv_obj_create(this->frm_graph_frame);
        lv_obj_set_size(node, 20, 20);
        lv_obj_set_style_radius(node, 10, 0);  // Círculo
        lv_obj_set_pos(node, (lv_coord_t)nodes[i].x, (lv_coord_t)nodes[i].y);
        lv_obj_set_style_bg_color(node, lv_color_hex(0xFF0000), LV_PART_MAIN); // Cor do nó
        lv_obj_t * label = lv_label_create(node);
        lv_label_set_text(label, nodes[i].id);
        lv_obj_set_style_text_color(label, lv_color_hex(0xffffff),  LV_PART_MAIN);
        lv_obj_set_align(label, LV_ALIGN_CENTER);
        lv_obj_clear_flag(node, LV_OBJ_FLAG_SCROLLABLE);
        pthread_mutex_unlock(this->lvgl_mutex);
    }
    /*
    // Desenha as arestas
    for (int i = 0; i < this->list_demo.size(); i++) {
        for (int j = 0; j < this->list_demo.size(); j++) {
            if (this->adjacency_matrix[i][j] == 1) {
                lv_obj_t * line = lv_line_create(this->frm_graph_frame);
                lv_point_t line_points[2];
                line_points[0].x = (float)this->nodes[i].x + 5; // Centro do nó
                line_points[0].y = (float)this->nodes[i].y + 5; // Centro do nó
                line_points[1].x = (float)this->nodes[j].x + 5; // Centro do nó
                line_points[1].y = (float)this->nodes[j].y + 5; // Centro do nó
                //printf("de (%d, %d) a (%d, %d)\n", (int)line_points[1].x, (int)line_points[1].y, (int)line_points[0].x, (int)line_points[0].y);
                //lv_obj_add_style(line, &style_line, LV_PART_MAIN);
                lv_line_set_points(line, line_points, 2);
            }
        }
    }
    */
}

void discovery_app::initUI(lv_obj_t * parent)
{
    this->parent = parent;
    if(this->parent){
        // Main form
        this->frm_discovery = lv_obj_create(this->parent);
        lv_obj_set_size(this->frm_discovery, LV_HOR_RES, LV_VER_RES);
        lv_obj_clear_flag(this->frm_discovery, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(this->frm_discovery, LV_OBJ_FLAG_HIDDEN);

        // Title button
        this->frm_discovery_btn_title = lv_btn_create(this->frm_discovery);
        lv_obj_set_size(this->frm_discovery_btn_title, 150, 20);
        lv_obj_align(this->frm_discovery_btn_title, LV_ALIGN_TOP_LEFT, -15, -15);
        lv_obj_add_event_cb(this->frm_discovery_btn_title, update_list, LV_EVENT_SHORT_CLICKED, this);

        this->frm_discovery_btn_title_lbl = lv_label_create(this->frm_discovery_btn_title);
        lv_label_set_text(this->frm_discovery_btn_title_lbl, (const char*)F("Discovered nodes"));
        lv_obj_set_align(this->frm_discovery_btn_title_lbl, LV_ALIGN_LEFT_MID);

        // Close button
        this->frm_discovery_btn_back = lv_btn_create(this->frm_discovery);
        lv_obj_set_size(this->frm_discovery_btn_back, 50, 20);
        lv_obj_align(this->frm_discovery_btn_back, LV_ALIGN_TOP_RIGHT, 15, -15);
        lv_obj_add_event_cb(this->frm_discovery_btn_back, hide, LV_EVENT_SHORT_CLICKED, this->frm_discovery);
        
        this->frm_discovery_btn_back_lbl = lv_label_create(this->frm_discovery_btn_back);
        lv_label_set_text(this->frm_discovery_btn_back_lbl, (const char*)F("Back"));
        lv_obj_set_align(this->frm_discovery_btn_back_lbl, LV_ALIGN_CENTER);

        this->init_graph_ui();
        // Graph button
        this->frm_discovery_btn_graph = lv_btn_create(this->frm_discovery);
        lv_obj_set_size(this->frm_discovery_btn_graph, 50, 20);
        lv_obj_align(this->frm_discovery_btn_graph, LV_ALIGN_TOP_RIGHT, -50, -15);
        lv_obj_add_event_cb(this->frm_discovery_btn_graph, show_graph, LV_EVENT_SHORT_CLICKED, this);

        this->frm_discovery_btn_graph_lbl = lv_label_create(this->frm_discovery_btn_graph);
        lv_label_set_text(this->frm_discovery_btn_graph_lbl, (const char*)F("Graph"));
        lv_obj_set_align(this->frm_discovery_btn_graph_lbl, LV_ALIGN_CENTER);

        // Add list object
        this->frm_discovery_nodeList = lv_list_create(this->frm_discovery);
        lv_obj_set_size(this->frm_discovery_nodeList, LV_HOR_RES, 220);
        lv_obj_align(this->frm_discovery_nodeList, LV_ALIGN_TOP_MID, 0, 5);
        lv_obj_set_style_border_opa(this->frm_discovery_nodeList, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(this->frm_discovery_nodeList, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        Serial.println(F("discovery_app::initUI() - READY"));
    }
    else
        Serial.println(F("discovery_app::initUI() - parent NULL"));
}

void discovery_app::init_graph_ui(){
    // Main grapgh form
    this->frm_graph_main = lv_obj_create(this->parent);
    lv_obj_set_size(this->frm_graph_main, 320, 240);
    lv_obj_set_style_bg_color(this->frm_graph_main, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(this->frm_graph_main, 255, LV_PART_MAIN);
    lv_obj_align(this->frm_graph_main, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(this->frm_graph_main, LV_OBJ_FLAG_SCROLLABLE);

    // Close button
    this->frm_graph_main_btn_back = lv_btn_create(this->frm_graph_main);
    lv_obj_set_size(this->frm_graph_main_btn_back, 50, 20);
    lv_obj_align(this->frm_graph_main_btn_back, LV_ALIGN_TOP_RIGHT, 15, -15);
    lv_obj_add_event_cb(this->frm_graph_main_btn_back, hide_graph, LV_EVENT_SHORT_CLICKED, this->frm_graph_main);
    
    this->frm_graph_main_btn_back_lbl = lv_label_create(this->frm_graph_main_btn_back);
    lv_label_set_text(this->frm_graph_main_btn_back_lbl, (const char*)F("Back"));
    lv_obj_set_align(this->frm_graph_main_btn_back_lbl, LV_ALIGN_CENTER);

    // Graph form
    this->frm_graph_frame = lv_obj_create(this->frm_graph_main);
    lv_obj_set_size(this->frm_graph_frame, 320, 200);
    lv_obj_set_style_bg_color(this->frm_graph_frame, lv_color_hex(0xffaa60), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(this->frm_graph_frame, 255, LV_PART_MAIN);
    lv_obj_align(this->frm_graph_frame, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_add_flag(this->frm_graph_main, LV_OBJ_FLAG_HIDDEN);

    Serial.printf((const char*)F("discovery_app::init_graph_ui() - READY\n"));

    // Demo
    disc_node n1, n2, n3, n4, n5, n6, n7, n8, n9, n10;
    strcpy(n1.id, "1");
    strcpy(n2.id, "2");
    strcpy(n3.id, "3");
    strcpy(n4.id, "4");
    strcpy(n5.id, "5");
    strcpy(n6.id, "6");
    strcpy(n7.id, "7");
    strcpy(n8.id, "8");
    strcpy(n9.id, "9");
    strcpy(n10.id, "10");

    strcpy(n1.neighbors[0], n2.id);
    strcpy(n1.neighbors[1], n4.id);

    strcpy(n2.neighbors[0], n1.id);
    strcpy(n2.neighbors[1], n3.id);
    strcpy(n2.neighbors[2], n5.id);

    strcpy(n3.neighbors[0], n2.id);
    strcpy(n3.neighbors[1], n6.id);

    strcpy(n4.neighbors[0], n1.id);
    strcpy(n4.neighbors[1], n5.id);
    strcpy(n4.neighbors[2], n7.id);

    strcpy(n5.neighbors[0], n2.id);
    strcpy(n5.neighbors[1], n4.id);
    strcpy(n5.neighbors[2], n6.id);
    strcpy(n5.neighbors[3], n8.id);

    strcpy(n6.neighbors[0], n3.id);
    strcpy(n6.neighbors[1], n5.id);
    strcpy(n6.neighbors[2], n9.id);

    strcpy(n7.neighbors[0], n4.id);
    strcpy(n7.neighbors[1], n8.id);

    strcpy(n8.neighbors[0], n5.id);
    strcpy(n8.neighbors[1], n7.id);
    strcpy(n8.neighbors[2], n9.id);

    strcpy(n9.neighbors[0], n6.id);
    strcpy(n9.neighbors[1], n8.id);

    strcpy(n10.neighbors[0], n3.id);

    this->list_demo.push_back(discovery_node(0, n1));
    this->list_demo.push_back(discovery_node(0, n2));
    this->list_demo.push_back(discovery_node(0, n3));
    this->list_demo.push_back(discovery_node(0, n4));
    this->list_demo.push_back(discovery_node(0, n5));
    this->list_demo.push_back(discovery_node(0, n6));
    this->list_demo.push_back(discovery_node(0, n7));
    this->list_demo.push_back(discovery_node(0, n8));
    this->list_demo.push_back(discovery_node(0, n9));
    //this->list_demo.push_back(discovery_node(0, n10));

    for(uint8_t r = 0; r < this->list_demo.size(); r++){
        printf("%s => ", this->list_demo[r].node.id);
        for(uint8_t c = 0; c < MAX_NEIGHBORS_COUNT; c++){
            printf("%s ", this->list_demo[r].node.neighbors[c]);
        }
        printf("\n");
    }

    this->init_positions();
    this->init_adj_matrix();
    this->calculate_adj_matrix();
}
