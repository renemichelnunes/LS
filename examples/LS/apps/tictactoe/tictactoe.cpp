#include <apps/tictactoe/tictactoe.hpp>
#include "tictactoe.hpp"

#define user_id "aaaaaa"
#define destiny_player "bbbbbb"

TaskHandle_t cpu_task = NULL;

static void hide(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    tictactoe * t = (tictactoe*)lv_event_get_user_data(e);
    if(code == LV_EVENT_SHORT_CLICKED){
        if(t->frm_main){
            lv_obj_add_flag(t->frm_main, LV_OBJ_FLAG_HIDDEN);
            if(cpu_task){
                vTaskDelete(cpu_task);
                cpu_task = NULL;
                Serial.println("cpu_task deleted");
            }
        }
    }
}

void new_game(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    tictactoe * ttt = (tictactoe*)lv_event_get_user_data(e);

    if(code == LV_EVENT_CLICKED){
        ttt->active = true;
        ttt->cpu_turn = false;
        ttt->init_board();
    }
}

void btn_event_handler(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    tictactoe * ttt = (tictactoe*)lv_event_get_user_data(e);
    char msg[30] = {'\0'};
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED){
        int row = lv_obj_get_y(btn) / 52;    // A posição do botão no eixo Y (linha)
        int col = lv_obj_get_x(btn) / 52;   // A posição do botão no eixo X (coluna)

        // Se o botão ainda estiver vazio e o jogo estiver ativoSem vencedores
        if (ttt->board[row][col] == ' ' && ttt->active) {
            // Atualiza o estado do botão
            ttt->board[row][col] = ttt->player;
            char p[2] = {'\0'};
            p[0] = ttt->player;
            // Definir o texto no botão (X ou O)
            lv_label_set_text(lv_obj_get_child(btn, 0), p);
            // Send the move
            if(ttt->player == 'X'){
                lora_packet lp;
                strcpy(lp.id, generate_ID(6).c_str());
                lp.app_id = APP_TICTACTOE;
                strcpy(lp.sender, user_id);
                strcpy(lp.destiny, destiny_player);
                lp.data_size = sizeof(ttt->mov);
                memcpy(lp.data, &ttt->mov, lp.data_size);
                lp.crc = calculate_data_crc(&ttt->mov, lp.data_size);
                printf("ID %s\nAPP_ID %d\nSender %s\nDestiny %s\nData size %d\n", lp.id, lp.app_id, lp.sender, lp.destiny, lp.data_size);
                uint8_t c = 0;
                for(uint8_t i = 0; i < sizeof(lp.data); i++){
                    printf("%x ", lp.data[i]);
                    c++;
                    if(c == 20){
                        printf("\n");
                        c = 0;
                    }
                }
                printf("\n");
                //lv_label_set_text(frame_game_lbl, "waiting the player...");
                //ttt->tpl->add(lp);
                if(ttt->multiplayer){
                    ttt->mov.row = row;
                    ttt->mov.col = col;
                    ttt->send_move = true;
                }
            }

            

            // Verificar se o jogador atual venceu
            if (ttt->checkVictory()) {
                if(ttt->player == 'X')
                    lv_label_set_text(ttt->frame_game_lbl, "You won!");
                else
                    lv_label_set_text(ttt->frame_game_lbl, "Better luck next time.");
                ttt->active = false;
                lv_obj_clear_flag(ttt->frame_game_new_btn, LV_OBJ_FLAG_HIDDEN);
            } else if (ttt->checkDraw()) {
                lv_label_set_text(ttt->frame_game_lbl, "Draw!");
                ttt->active = false;
                lv_obj_clear_flag(ttt->frame_game_new_btn, LV_OBJ_FLAG_HIDDEN);
            } else {
                // Change player
                ttt->player = (ttt->player == 'X') ? 'O' : 'X';
                if(ttt->player == 'X'){
                    strcpy(msg, "Your turn.");

                }
                else{
                    strcpy(msg, "Friend's turn.");
                    ttt->cpu_turn = true;
                }
                lv_label_set_text(ttt->frame_game_lbl, msg);
            }
        }
    }
}

void tictactoe::initUI(lv_obj_t * parent)
{
    // Main form
    this->parent = parent;
    if(this->parent){
         // Main form
        this->frm_main = lv_obj_create(this->parent);
        lv_obj_set_size(this->frm_main, 320, 240);
        //lv_obj_set_style_bg_color(frm_main, lv_color_hex(0xffffff), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(this->frm_main, 255, LV_PART_MAIN);
        lv_obj_align(this->frm_main, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_radius(this->frm_main, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(this->frm_main, 0, LV_PART_MAIN);
        lv_obj_clear_flag(this->frm_main, LV_OBJ_FLAG_SCROLLABLE);

        this->frm_main_title_btn = lv_btn_create(this->frm_main);
        lv_obj_set_size(this->frm_main_title_btn, 310, 20);
        lv_obj_align(this->frm_main_title_btn, LV_ALIGN_TOP_LEFT, -10, -10);

        this->frm_main_title_btn_lbl = lv_label_create(this->frm_main_title_btn);
        lv_label_set_text(this->frm_main_title_btn_lbl, "TIC-TAC-TOE");
        lv_obj_set_align(this->frm_main_title_btn_lbl, LV_ALIGN_LEFT_MID);

        this->frm_main_back_btn = lv_btn_create(this->frm_main);
        lv_obj_set_size(this->frm_main_back_btn, 50, 20);
        lv_obj_align(this->frm_main_back_btn, LV_ALIGN_TOP_RIGHT, 10, -10);
        lv_obj_add_event_cb(this->frm_main_back_btn, hide, LV_EVENT_SHORT_CLICKED, this);

        this->frm_main_back_btn_lbl = lv_label_create(this->frm_main_back_btn);
        lv_label_set_text(this->frm_main_back_btn_lbl, "Back");
        lv_obj_set_align(this->frm_main_back_btn_lbl, LV_ALIGN_CENTER);

        // Frame dos nós
        this->frame_game = lv_obj_create(this->frm_main);
        lv_obj_set_size(this->frame_game, 320, 210);
        lv_obj_set_style_bg_color(this->frame_game, lv_color_hex(0xeeeeee), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(this->frame_game, 255, LV_PART_MAIN);
        lv_obj_align(this->frame_game, LV_ALIGN_TOP_MID, 0, 15);
        lv_obj_set_style_radius(this->frame_game, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(this->frame_game, 0, LV_PART_MAIN);
        lv_obj_set_scroll_dir(this->frame_game, LV_DIR_VER);
        
        lv_obj_add_flag(this->frm_main, LV_OBJ_FLAG_HIDDEN);

        this->init_board();
        Serial.println("tictactoe_app::initUI() - READY");
    }else
        Serial.println("tictactoe_app::initUI() - parent NULL");
}

static void cpu_move(void * ttt)
{
    tictactoe * t = (tictactoe*)ttt;
    while(true){
        if(t->cpu_turn){
            uint8_t row = rand() % 3;
            uint8_t col = rand() % 3;
            t->simulate_click(row, col);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void tictactoe::showUI()
{
    if(this->frm_main){
        lv_obj_clear_flag(this->frm_main, LV_OBJ_FLAG_HIDDEN);
        
        if(cpu_task == NULL){
            xTaskCreatePinnedToCore(cpu_move, "cpu_move", 6000, this, 1, &cpu_task, 1);
            Serial.println("cpu_task launched");
        }
    }
}

bool tictactoe::checkVictory()
{
    // Check rows
    for(uint8_t i = 0; i < 3; i++){
        if(this->board[i][0] == this->player && this->board[i][1] == this->player && this->board[i][2] == this->player)
        return true;
    }
    // Check columns
    for(uint8_t i = 0; i < 3; i++){
        if(this->board[0][i] == this->player && this->board[1][i] == this->player && this->board[2][i] == this->player)
        return true;
    }
    // Check diagonals
    if(this->board[0][0] == this->player && this->board[1][1] == this->player && this->board[2][2] == this->player)
        return true;
    if(this->board[0][2] == this->player && this->board[1][1] == this->player && this->board[2][0] == this->player)
        return true;
    return false;
}

bool tictactoe::checkDraw()
{
    for(uint8_t i = 0; i < 3; i++){
        for(uint8_t j = 0; j < 3; j++){
            if(this->board[i][j] == ' ')
                return false;
        }
    }
    return true;
}

void tictactoe::simulate_click(uint8_t row, uint8_t col)
{
    lv_obj_t * btn;

    if(this->board[row][col] == ' '){
        btn = this->btns[row][col];
        lv_event_send(btn, LV_EVENT_CLICKED, NULL);
        this->cpu_turn = false;
    }
}

void tictactoe::init_board()
{
    this->player = 'X';
    this->cpu_turn = false;
    this->active = true;
    lv_obj_clean(this->frame_game);
    // Inicializar o tabuleiro com 9 botões
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            this->btns[i][j] = lv_btn_create(this->frame_game);
            lv_obj_set_size(this->btns[i][j], 50, 50);
            lv_obj_set_pos(this->btns[i][j], j * 52, i * 52); // Definir a posição do botão
            lv_obj_add_event_cb(this->btns[i][j], btn_event_handler, LV_EVENT_CLICKED, this);
            lv_obj_t* label = lv_label_create(this->btns[i][j]);
            lv_label_set_text(label, ""); // Inicialmente, os botões estão vazios
        }
    }

    // Inicializar o tabuleiro
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            this->board[i][j] = ' ';
        }
    }

    this->frame_game_lbl = lv_label_create(this->frame_game);
    lv_obj_align(this->frame_game_lbl, LV_ALIGN_BOTTOM_LEFT, 0,0);
    lv_label_set_text(this->frame_game_lbl, "Let's play!");

    this->frame_game_new_btn = lv_btn_create(this->frame_game);
    lv_obj_set_size(this->frame_game_new_btn, 40, 20);
    lv_obj_align(this->frame_game_new_btn, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_add_event_cb(this->frame_game_new_btn, new_game, LV_EVENT_CLICKED, this);

    this->frame_game_new_btn_lbl = lv_label_create(this->frame_game_new_btn);
    lv_label_set_text(this->frame_game_new_btn_lbl, "New");
    lv_obj_set_align(this->frame_game_new_btn_lbl, LV_ALIGN_CENTER);
    lv_obj_add_flag(this->frame_game_new_btn, LV_OBJ_FLAG_HIDDEN);
}

tictactoe::tictactoe(lora_outgoing_packets * l)
{
    this->tpl = tpl;
    for(uint8_t i = 0; i < 3; i++)
        for(uint8_t j = 0; j < 3; j++)
            this->board[i][j] = ' ';

    for(uint8_t i = 0; i < 3; i++){
        for(uint8_t j = 0; j < 3; j++){
            printf("%c ", this->board[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    xTaskCreatePinnedToCore(cpu_move, "cpu_task", 6000, this, 1, &cpu_task, 1);
    if(cpu_task){
        Serial.println("cpu_task launched");
    }
}

ttt_mov tictactoe::getMove()
{
    this->send_move = false;
    return this->mov;
}

static bool cmp_name( ttt_player &c1,  ttt_player &c2){
    return c1.name < c2.name;
}

bool tictactoe::add_player(ttt_player p)
{
    for(ttt_player player : this->ttt_players){
        if(strcmp(player.id, p.id) == 0){
            return false;
        }
    }
    this->ttt_players.push_back(p);
    std::sort(this->ttt_players.begin(), this->ttt_players.end(), cmp_name);
    return true;
}

bool tictactoe::del_player(ttt_player p)
{
    for(uint8_t i = 0; i < this->ttt_players.size(); i++){
        if(strcmp(this->ttt_players[i].id, p.id) == 0){
            this->ttt_players.erase(this->ttt_players.begin() + i);
            return true;
        }
    }
    return false;
}
