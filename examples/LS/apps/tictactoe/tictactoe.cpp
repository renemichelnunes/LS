#include <apps/tictactoe/tictactoe.hpp>
#include "tictactoe.hpp"

static void hide(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * frm = (lv_obj_t*)lv_event_get_user_data(e);
    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm){
            lv_obj_add_flag(frm, LV_OBJ_FLAG_HIDDEN);
            
        }
    }
}

static void btn_event_handler(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    tictactoe * tic = (tictactoe*)lv_event_get_user_data(e);
    int row = lv_obj_get_y(btn) / 15;    // A posição do botão no eixo Y (linha)
    int col = lv_obj_get_x(btn) / 15;   // A posição do botão no eixo X (coluna)

    // Se o botão ainda estiver vazio e o jogo estiver ativo
    if (tic->board[row][col] == ' ' && tic->active) {
        // Atualiza o estado do botão
        tic->board[row][col] = tic->player;
        
        // Definir o texto no botão (X ou O)
        lv_label_set_text(lv_obj_get_child(btn, 0), &tic->player);

        // Verificar se o jogador atual venceu
        if (tic->checkVictory()) {
            lv_label_set_text(tic->frm_tictactoe_btn_title_lbl, "You won!");
            tic->active = false;
        } else if (tic->checkDraw()) {
            lv_label_set_text(tic->frm_tictactoe_btn_title_lbl, "Draw!");
            tic->active = false;
        } else {
            // Trocar jogador
            tic->player = (tic->player == 'X') ? 'O' : 'X';
        }
    }
}

void tictactoe::initUI(lv_obj_t * parent)
{
    // Main form
    this->parent = parent;
    if(this->parent){
        this->frm_tictactoe = lv_obj_create(this->parent);
        lv_obj_set_size(this->frm_tictactoe, LV_HOR_RES, LV_VER_RES);
        lv_obj_clear_flag(this->frm_tictactoe, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(this->frm_tictactoe, LV_OBJ_FLAG_HIDDEN);
        // Title button
        this->frm_tictactoe_btn_title = lv_btn_create(this->frm_tictactoe);
        lv_obj_set_size(this->frm_tictactoe_btn_title, 200, 20);
        lv_obj_align(this->frm_tictactoe_btn_title, LV_ALIGN_TOP_LEFT, -15, -15);

        this->frm_tictactoe_btn_title_lbl = lv_label_create(this->frm_tictactoe_btn_title);
        lv_label_set_text(this->frm_tictactoe_btn_title_lbl, "TIC TAC TOE");
        lv_obj_set_align(this->frm_tictactoe_btn_title_lbl, LV_ALIGN_LEFT_MID);

        // Close button
        this->frm_tictactoe_btn_back = lv_btn_create(this->frm_tictactoe);
        lv_obj_set_size(this->frm_tictactoe_btn_back, 50, 20);
        lv_obj_align(this->frm_tictactoe_btn_back, LV_ALIGN_TOP_RIGHT, 15, -15);
        lv_obj_add_event_cb(this->frm_tictactoe_btn_back, hide, LV_EVENT_SHORT_CLICKED, this->frm_tictactoe);
        
        this->frm_tictactoe_btn_back_lbl = lv_label_create(this->frm_tictactoe_btn_back);
        lv_label_set_text(this->frm_tictactoe_btn_back_lbl, "Back");
        lv_obj_set_align(this->frm_tictactoe_btn_back_lbl, LV_ALIGN_CENTER);

        // Set the board
        this->frm_tictactoe_board = lv_obj_create(this->frm_tictactoe);
        lv_obj_set_size(this->frm_tictactoe_board, 200, 200);
        lv_obj_align(this->frm_tictactoe_board, LV_ALIGN_TOP_LEFT, -15, -15);
        lv_obj_set_style_bg_color(this->frm_tictactoe_board, lv_color_hex(0xffaa00), LV_PART_MAIN);

        // Put the buttons
        for(uint8_t i = 0; i < 3; i++){
            for(uint8_t j = 0; j < 3; j++){
                this->btns[i][j] = lv_btn_create(this->frm_tictactoe_board);
                lv_obj_set_size(this->btns[i][j], 15, 15);
                lv_obj_set_pos(this->btns[i][j], j * 15, i * 15); // Definir a posição do botão
                lv_obj_add_event_cb(this->btns[i][j], btn_event_handler, LV_EVENT_CLICKED, this);
                lv_obj_t* label = lv_label_create(this->btns[i][j]);
                lv_label_set_text(label, "");
            }
        }

        Serial.println("tictactoe_app::initUI() - READY");
    }else
        Serial.println("tictactoe_app::initUI() - parent NULL");
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
