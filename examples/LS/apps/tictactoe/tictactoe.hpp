#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include "lora_messages.hpp"
#include <vector>

#define APP_TICTACTOE 4

struct ttt_mov{
    uint8_t row = 0;
    uint8_t col = 0;
};

struct ttt_player{
    char id[7] = {'\0'};
    char name[50] ={'\0'};
};

class tictactoe{
    private:
        std::vector<ttt_player> ttt_players;
        ttt_player * actual_player;
    public:
    // UI objects
    lv_obj_t * frm_main;
    lv_obj_t * frm_main_title_btn;
    lv_obj_t * frm_main_title_btn_lbl;
    lv_obj_t * frm_main_back_btn;
    lv_obj_t * frm_main_back_btn_lbl;
    lv_obj_t * frame_game;
    lv_obj_t * frame_game_lbl;
    lv_obj_t * frame_game_new_btn;
    lv_obj_t * frame_game_new_btn_lbl;
    lv_obj_t * frm_tictactoe_players;
    lv_obj_t * frm_tictactoe_players_btn_title;
    lv_obj_t * frm_tictactoe_players_btn_title_lbl;
    lv_obj_t * frm_tictactoe_players_btn_back;
    lv_obj_t * frm_tictactoe_players_btn_back_lbl;
    lv_obj_t * btns[3][3];
    lv_obj_t * parent;

    char board[3][3];
    char player = 'X';
    bool active = true;
    bool cpu_turn = false;
    bool send_move = false;
    bool multiplayer = false;
    ttt_mov mov;

    lora_outgoing_packets * tpl;

    bool checkVictory();
    bool checkDraw();
    void initUI(lv_obj_t * parent);
    void showUI();
    void init_board();
    void simulate_click(uint8_t row, uint8_t col);
    tictactoe(lora_outgoing_packets * tpl);
    ttt_mov getMove();
    //~tictactoe();
    bool add_player(ttt_player p);
    bool del_player(ttt_player p);
};
