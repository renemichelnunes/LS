#include <apps/tictactoe/tictactoe.hpp>
#include "lora_messages.hpp"
#include "tictactoe.hpp"

ttt_player * actual_player;
TaskHandle_t cpu_task = NULL;
TaskHandle_t task_ttt_evt_handler = NULL;

void accept_inv(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    tictactoe * ttt = (tictactoe*)lv_event_get_user_data(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        ttt->online = true;
        ttt->cpu_turn = false;
        ttt->init_board();
        lv_label_set_text(ttt->frame_game_lbl, "Wait your friend do the first move...");
        lv_obj_clear_flag(ttt->frame_game_block, LV_OBJ_FLAG_HIDDEN);
        ttt->player = 'O';

        ttt->tttp.packet_type = TTT_TYPE_INVITATION;
        ttt->tttp.invitation_type = TTT_INVITATION_TYPE_ACCEPTED;
        strcpy(ttt->tttp.player_id, ttt->user_id);
        ttt->packet_ready = true;
        ttt->invitation_requested = false;
    }
}

void decline_inv(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    tictactoe * ttt = (tictactoe*)lv_event_get_user_data(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        lv_obj_add_flag(ttt->frame_game_invitation_frame, LV_OBJ_FLAG_HIDDEN);
        ttt->tttp.packet_type = TTT_TYPE_INVITATION;
        ttt->tttp.invitation_type = TTT_INVITATION_TYPE_DECLINED;
        strcpy(ttt->tttp.player_id, ttt->user_id);
        ttt->packet_ready = true;
        ttt->invitation_requested = false;
    }
}

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
            if(task_ttt_evt_handler){
                vTaskDelete(task_ttt_evt_handler);
                task_ttt_evt_handler = NULL;
                Serial.println("task_ttt_evt_handler deleted");
            }
        }
    }
}

void new_game(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    tictactoe * ttt = (tictactoe*)lv_event_get_user_data(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        lv_obj_add_flag(ttt->frame_game_invitation_frame, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ttt->frame_game_online_list, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ttt->frame_game_new_game_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ttt->frame_game_new_mpu_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ttt->frame_game_new_friend_btn, LV_OBJ_FLAG_HIDDEN);
        ttt->invitation_requested = false;
    }
}

void ttt_event_handler(void * p){
    tictactoe * ttt = (tictactoe*)p;
    while(true){
        if(ttt->packet_ready){
            lora_packet lp;
            strcpy(lp.id, generate_ID(6).c_str());
            lp.app_id = APP_TICTACTOE;
            strcpy(lp.sender, ttt->tttp.player_id);
            if(ttt->selected_player)
                strcpy(lp.destiny, ttt->selected_player->id);
            lp.type = LORA_PKT_DATA;
            lp.data_size = sizeof(ttt->tttp);
            memcpy(lp.data, &ttt->tttp, lp.data_size);

            if(ttt->tttp.packet_type == TTT_TYPE_INVITATION){
                if(ttt->selected_player)
                    printf("Selected player\nID %s\nName %s\n\n", ttt->selected_player->id, ttt->selected_player->name);
                switch(ttt->tttp.invitation_type){
                    case TTT_INVITATION_TYPE_ACCEPTED:
                        printf("Invitation accepted\n");
                        break;
                    case TTT_INVITATION_TYPE_DECLINED:
                        printf("Invitation declined\n");
                        break;
                    case TTT_INVITATION_TYPE_BUSY:
                        printf("Invitation declined, busy\n");
                        break;
                    case TTT_INVITATION_TYPE_REQUEST:
                        if(ttt->selected_player){
                            printf("Invitation sent to %s\n", ttt->selected_player->name);
                            lv_label_set_text(ttt->frame_game_lbl, "Waiting the player...");
                            lv_obj_clear_flag(ttt->frame_game_block, LV_OBJ_FLAG_HIDDEN);
                        }
                        else
                            printf("Invitation sent without selecting a player\n");
                        break;
                    case TTT_INVITATION_TYPE_UNKNOWN:
                        printf("Invitation type unknown\n");
                        break;
                }
            }

            ttt->transmit_list_add_callback(lp);
            ttt->packet_ready = false;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void btn_event_handler(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    tictactoe * ttt = (tictactoe*)lv_event_get_user_data(e);
    char msg[30] = {'\0'};
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED){
        int linha = lv_obj_get_y(btn) / 52;    // A posição do botão no eixo Y (linha)
        int coluna = lv_obj_get_x(btn) / 52;   // A posição do botão no eixo X (coluna)

        ttt_mov mov;
        mov.row = linha;
        mov.col = coluna;

        // Se o botão ainda estiver vazio e o jogo estiver ativoSem vencedores
        if (ttt->board[linha][coluna] == ' ' && ttt->active) {
            // Atualiza o estado do botão
            ttt->board[linha][coluna] = ttt->player;
            char p[2] = {'\0'};
            p[0] = ttt->player;
            // Definir o texto no botão (X ou O)
            lv_label_set_text(lv_obj_get_child(btn, 0), p);
            // Send the move
            if(ttt->player == 'X'){
                lora_packet lp;
                strcpy(lp.id, generate_ID(6).c_str());
                lp.app_id = APP_TICTACTOE;
                strcpy(lp.sender, ttt->user_id);
                strcpy(lp.destiny, ttt->selected_player->id);
                lp.data_size = sizeof(mov);
                memcpy(lp.data, &mov, lp.data_size);
                lp.crc = calculate_data_crc(&mov, lp.data_size);
                lp.confirmed = false;
                printf("\nSending move\nID %s\nAPP_ID %d\nSender %s\nDestiny %s\nData size %d\n", lp.id, lp.app_id, lp.sender, lp.destiny, lp.data_size);
                ttt->transmit_list_add_callback(lp);
                /*
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
                */
            }

            //lv_label_set_text(frame_game_lbl, "waiting the player...");

            // Verificar se o jogador atual venceu
            if (ttt->checkVictory()) {
                if(ttt->player == 'X')
                    lv_label_set_text(ttt->frame_game_lbl, "You won!");
                else
                    lv_label_set_text(ttt->frame_game_lbl, "Better luck next time.");
                ttt->active = false;
                lv_obj_clear_flag(ttt->frame_game_new_game_btn, LV_OBJ_FLAG_HIDDEN);
            } else if (ttt->checkDraw()) {
                lv_label_set_text(ttt->frame_game_lbl, "Draw!");
                ttt->active = false;
                lv_obj_clear_flag(ttt->frame_game_new_game_btn, LV_OBJ_FLAG_HIDDEN);
            } else {
                // Change player
                ttt->player = (ttt->player == 'X') ? 'O' : 'X';
                if(ttt->player == 'X'){
                    strcpy(msg, "Your turn.");
                    lv_obj_add_flag(ttt->frame_game_block, LV_OBJ_FLAG_HIDDEN);
                }
                else{
                    strcpy(msg, "Friend's turn.");
                    ttt->cpu_turn = true;
                    lv_obj_clear_flag(ttt->frame_game_block, LV_OBJ_FLAG_HIDDEN);
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
        lv_obj_align(this->frm_main, LV_ALIGN_TOP_MID, -10, -15);
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

        this->frm_main_back_btn_lbl = lv_label_create(this->frm_main_back_btn);
        lv_label_set_text(this->frm_main_back_btn_lbl, "Back");
        lv_obj_set_align(this->frm_main_back_btn_lbl, LV_ALIGN_CENTER);

        this->frame_game = lv_obj_create(this->frm_main);
        lv_obj_set_size(this->frame_game, 320, 210);
        lv_obj_set_style_bg_color(this->frame_game, lv_color_hex(0xeeeeee), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(this->frame_game, 255, LV_PART_MAIN);
        lv_obj_align(this->frame_game, LV_ALIGN_TOP_MID, 0, 15);
        lv_obj_set_style_radius(this->frame_game, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(this->frame_game, 0, LV_PART_MAIN);
        lv_obj_set_scroll_dir(this->frame_game, LV_DIR_VER);

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

void new_game_cpu(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    tictactoe * ttt = (tictactoe*)lv_event_get_user_data(e);

    if(code == LV_EVENT_CLICKED){
        ttt->active = true;
        ttt->cpu_turn = false;
        ttt->online = false;
        ttt->init_board();
        ttt->player = 'X';
    }
}

void tictactoe::showUI(){
    if(this->frm_main){
        lv_obj_clear_flag(this->frm_main, LV_OBJ_FLAG_HIDDEN);
        // TTT event handler thread
        if(!task_ttt_evt_handler){
            xTaskCreatePinnedToCore(ttt_event_handler, "ttt_evt_hand", 6000, this, 1, &task_ttt_evt_handler, 1);
            if(task_ttt_evt_handler)
                Serial.printf("task_ttt_evt_handler launched\n");
            else
                Serial.printf("task_ttt_evt_handler failed to launch\n");
        }
    }
}

void tictactoe::hideUI()
{
    if(this->frm_main){
        if(task_ttt_evt_handler){
            vTaskDelete(task_ttt_evt_handler);
            task_ttt_evt_handler = NULL;
            Serial.printf("task_ttt_evt_handler deleted\n");
        }
        lv_obj_add_flag(this->frm_main, LV_OBJ_FLAG_HIDDEN);
    }
}

ttt_player *tictactoe::get_player_by_id(const char *id)
{
    for(uint8_t i = 0; i < this->ttt_players.size(); i++){
        if(strcmp(this->ttt_players[i].id, id) == 0){
            return &this->ttt_players[i];
        }
    }
    return NULL;
}

void select_player(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = (lv_obj_t*)lv_event_get_target(e);
    tictactoe * ttt = (tictactoe*)lv_event_get_user_data(e);
    actual_player = (ttt_player*)lv_event_get_user_data(e);
    char id[7] = {'\0'};

    if(code == LV_EVENT_SHORT_CLICKED){
        strcpy(id, lv_label_get_text(lv_obj_get_child(btn, 2)));
        ttt->selected_player = ttt->get_player_by_id(id);
        ttt->tttp.packet_type = TTT_TYPE_INVITATION;
        ttt->tttp.invitation_type = TTT_INVITATION_TYPE_REQUEST;
        ttt->packet_ready = true;
    }
}

void tictactoe::refresh_players_list()
{
    lv_obj_t * btn = NULL;
    if(this->frame_game_online_list){
        lv_obj_clean(this->frame_game_online_list);
        for(uint8_t i = 0; i < this->ttt_players.size(); i++){
            btn = lv_list_add_btn(this->frame_game_online_list, LV_SYMBOL_WIFI, this->ttt_players[i].name);
            lv_obj_t * id_lbl = lv_label_create(btn);
            lv_obj_add_event_cb(btn, select_player, LV_EVENT_SHORT_CLICKED, this);

            lv_label_set_text(id_lbl, this->ttt_players[i].id);
            lv_obj_add_flag(id_lbl, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void show_online(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    tictactoe* ttt = (tictactoe*)lv_event_get_user_data(e);

    if(code == LV_EVENT_CLICKED){
        lv_obj_add_flag(ttt->frame_game_new_mpu_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ttt->frame_game_new_friend_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ttt->frame_game_new_game_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ttt->frame_game_online_list, LV_OBJ_FLAG_HIDDEN);
        ttt->refresh_players_list();
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

    this->frame_game_new_mpu_btn = lv_btn_create(this->frame_game);
    lv_obj_set_size(this->frame_game_new_mpu_btn, 120, 20);
    lv_obj_align(this->frame_game_new_mpu_btn, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_add_event_cb(this->frame_game_new_mpu_btn, new_game_cpu, LV_EVENT_CLICKED, this);
    lv_obj_add_flag(this->frame_game_new_mpu_btn, LV_OBJ_FLAG_HIDDEN);

    this->frame_game_new_mpu_btn_lbl = lv_label_create(this->frame_game_new_mpu_btn);
    lv_label_set_text(this->frame_game_new_mpu_btn_lbl, "New with mpu");
    lv_obj_set_align(this->frame_game_new_mpu_btn_lbl, LV_ALIGN_CENTER);

    this->frame_game_new_friend_btn = lv_btn_create(this->frame_game);
    lv_obj_set_size(this->frame_game_new_friend_btn, 130, 20);
    lv_obj_align(this->frame_game_new_friend_btn, LV_ALIGN_TOP_RIGHT, 0, 30);
    lv_obj_add_event_cb(this->frame_game_new_friend_btn, show_online, LV_EVENT_CLICKED, this);
    lv_obj_add_flag(this->frame_game_new_friend_btn, LV_OBJ_FLAG_HIDDEN);

    this->frame_game_new_friend_btn_lbl = lv_label_create(this->frame_game_new_friend_btn);
    lv_label_set_text(this->frame_game_new_friend_btn_lbl, "New with friend");
    lv_obj_set_align(this->frame_game_new_friend_btn_lbl, LV_ALIGN_CENTER);

    this->frame_game_new_game_btn = lv_btn_create(this->frame_game);
    lv_obj_set_size(this->frame_game_new_game_btn, 40, 20);
    lv_obj_align(this->frame_game_new_game_btn, LV_ALIGN_TOP_RIGHT, 0, -15);
    lv_obj_add_event_cb(this->frame_game_new_game_btn, new_game, LV_EVENT_SHORT_CLICKED, this);

    this->frame_game_new_game_btn_lbl = lv_label_create(this->frame_game_new_game_btn);
    lv_label_set_text(this->frame_game_new_game_btn_lbl, "New");
    lv_obj_set_align(this->frame_game_new_game_btn_lbl, LV_ALIGN_CENTER);

    this->frame_game_online_list = lv_list_create(this->frame_game);
    lv_obj_set_size(this->frame_game_online_list, 140, 145);
    lv_obj_set_scroll_dir(this->frame_game_online_list, LV_DIR_VER);
    lv_obj_align(this->frame_game_online_list, LV_ALIGN_TOP_RIGHT, 15, 10);
    lv_obj_set_style_bg_color(this->frame_game_online_list, lv_color_hex(0xeeeeee), LV_PART_MAIN);
    lv_obj_add_flag(this->frame_game_online_list, LV_OBJ_FLAG_HIDDEN);

    this->frame_game_invitation_frame = lv_obj_create(this->frame_game);
    lv_obj_set_size(this->frame_game_invitation_frame, 130, 100);
    lv_obj_align(this->frame_game_invitation_frame, LV_ALIGN_TOP_RIGHT, 15, 10);
    lv_obj_clear_flag(this->frame_game_invitation_frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(this->frame_game_invitation_frame, LV_OBJ_FLAG_HIDDEN);

    this->frame_game_invitation_frame_lbl = lv_label_create(this->frame_game_invitation_frame);
    lv_obj_set_size(this->frame_game_invitation_frame_lbl, 110, LV_SIZE_CONTENT);
    lv_label_set_long_mode(this->frame_game_invitation_frame_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_align(this->frame_game_invitation_frame_lbl, LV_ALIGN_TOP_MID, 0, -15);

    this->frame_game_invitation_frame_btn_accept = lv_btn_create(this->frame_game_invitation_frame);
    lv_obj_set_size(this->frame_game_invitation_frame_btn_accept, 50, 20);
    lv_obj_align(this->frame_game_invitation_frame_btn_accept, LV_ALIGN_TOP_LEFT, -15, 50);
    lv_obj_add_event_cb(this->frame_game_invitation_frame_btn_accept, accept_inv, LV_EVENT_SHORT_CLICKED, this);

    this->frame_game_invitation_frame_btn_accept_lbl = lv_label_create(this->frame_game_invitation_frame_btn_accept);
    lv_label_set_text(this->frame_game_invitation_frame_btn_accept_lbl, "Yes");
    lv_obj_set_align(this->frame_game_invitation_frame_btn_accept_lbl, LV_ALIGN_CENTER);

    this->frame_game_invitation_frame_btn_decline = lv_btn_create(this->frame_game_invitation_frame);
    lv_obj_set_size(this->frame_game_invitation_frame_btn_decline, 50, 20);
    lv_obj_align(this->frame_game_invitation_frame_btn_decline, LV_ALIGN_TOP_LEFT, 50, 50);
    lv_obj_add_event_cb(this->frame_game_invitation_frame_btn_decline, decline_inv, LV_EVENT_SHORT_CLICKED, this);

    this->frame_game_invitation_frame_btn_decline_lbl = lv_label_create(this->frame_game_invitation_frame_btn_decline);
    lv_label_set_text(this->frame_game_invitation_frame_btn_decline_lbl, "No");
    lv_obj_set_align(this->frame_game_invitation_frame_btn_decline_lbl, LV_ALIGN_CENTER);

    this->frame_game_block = lv_obj_create(frame_game);
    lv_obj_set_size(this->frame_game_block, 154, 156);
    lv_obj_align(this->frame_game_block, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(this->frame_game_block, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(this->frame_game_block, 80, LV_PART_MAIN);
    lv_obj_add_flag(this->frame_game_block, LV_OBJ_FLAG_HIDDEN);
}

tictactoe::tictactoe(void (*transmit_list_add_callback)(lora_packet))
{
    this->transmit_list_add_callback = transmit_list_add_callback;
    
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

ttt_packet tictactoe::process_packet(ttt_packet p)
{
    char msg[100] = {'\0'};

    if(p.packet_type == TTT_TYPE_INVITATION){
        if(p.invitation_type == TTT_INVITATION_TYPE_ACCEPTED){
            this->invitation_accepted = true;
            this->online = true;
            this->cpu_turn = false;
            this->init_board();

            lv_label_set_text(this->frame_game_lbl, "Let's go!");
            lv_obj_add_flag(this->frame_game_online_list, LV_OBJ_FLAG_HIDDEN);
        }
        else if(p.invitation_type == TTT_INVITATION_TYPE_DECLINED){
            this->invitation_accepted = false;
            this->online = false;
            lv_label_set_text(this->frame_game_lbl, "Invitation declined");
            lv_obj_add_flag(this->frame_game_online_list, LV_OBJ_FLAG_HIDDEN);
        }
        else if(p.invitation_type == TTT_INVITATION_TYPE_UNKNOWN){
            this->invitation_accepted = false;
        }
    }
    else if(p.packet_type == TTT_TYPE_MOVE){
        if(p.mov.row > 2 || p.mov.col > 2 || p.mov.row < 0 || p.mov.col < 0){
            printf("Invalid row or column\n");
            return p;
        }
        this->simulate_click(p.mov.row, p.mov.col);
    }
    else if(p.packet_type == TTT_TYPE_DISCONNECT){
        this->selected_player = NULL;
        this->online = false;
        lv_label_set_text(this->frame_game_lbl, "I need a coffee, bye");
    }
    else if(p.packet_type == TTT_TYPE_REQUEST){
        ttt_player * tp = this->get_player_by_id(p.player_id);
        if(!tp)
            return p;
        if(!this->invitation_requested){
            this->invitation_requested = true;
            sprintf(msg, "%s wants to play with you\n", tp->name);
            printf("%s", msg);
            lv_label_set_text(this->frame_game_invitation_frame_lbl, msg);
            lv_obj_clear_flag(this->frame_game_invitation_frame, LV_OBJ_FLAG_HIDDEN);
        }
        else{
            printf("%s wait your turn\n", this->get_player_by_id(p.player_id)->name);
        }
    }
    else if(p.packet_type == TTT_TYPE_ACK){

    }
    return p;
}
