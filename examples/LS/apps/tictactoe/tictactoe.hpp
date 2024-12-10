#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include "l2l.hpp"
#include <vector>

#define APP_TICTACTOE 4

#define TTT_TYPE_UNKNOWN 0
#define TTT_TYPE_INVITATION 1
#define TTT_TYPE_MOVE 2
#define TTT_TYPE_DISCONNECT 3
#define TTT_TYPE_ACK 4
#define TTT_TYPE_REQUEST 4

#define TTT_INVITATION_TYPE_UNKNOWN 0
#define TTT_INVITATION_TYPE_DECLINED 1
#define TTT_INVITATION_TYPE_ACCEPTED 2
#define TTT_INVITATION_TYPE_BUSY 3
#define TTT_INVITATION_TYPE_REQUEST 4

struct ttt_mov{
    uint8_t row = 0;
    uint8_t col = 0;
};

struct ttt_player{
    char id[7] = {'\0'};
    char name[50] ={'\0'};
};

struct ttt_packet{
    char player_id[7] = {'\0'};
    uint8_t packet_type;
    uint8_t invitation_type;
    ttt_mov mov;
};

class tictactoe{
    private:
        
    public:
    // UI objects
    lv_obj_t * frm_main;
    lv_obj_t * frm_main_title_btn;
    lv_obj_t * frm_main_title_btn_lbl;
    lv_obj_t * frm_main_back_btn;
    lv_obj_t * frm_main_back_btn_lbl;

    lv_obj_t * frame_game;
    lv_obj_t * frame_game_lbl;
    lv_obj_t * frame_game_new_mpu_btn;
    lv_obj_t * frame_game_new_mpu_btn_lbl;
    lv_obj_t * frame_game_new_friend_btn;
    lv_obj_t * frame_game_new_friend_btn_lbl;
    lv_obj_t * frame_game_new_game_btn;
    lv_obj_t * frame_game_new_game_btn_lbl;
    lv_obj_t * frame_game_block;

    // Online players
    lv_obj_t * frame_game_online_btn_back;
    lv_obj_t * frame_game_online_btn_back_lbl;
    lv_obj_t * frame_game_online_list;

    // Accept/Decline invitation_accepted
    lv_obj_t * frame_game_invitation_frame;
    lv_obj_t * frame_game_invitation_frame_lbl;
    lv_obj_t * frame_game_invitation_frame_btn_accept;
    lv_obj_t * frame_game_invitation_frame_btn_accept_lbl;
    lv_obj_t * frame_game_invitation_frame_btn_decline;
    lv_obj_t * frame_game_invitation_frame_btn_decline_lbl;

    lv_obj_t * btns[3][3];
    lv_obj_t * parent;

    char board[3][3];
    char player = 'X';
    volatile bool active = true;
    volatile bool cpu_turn = false;
    volatile bool online = false;
    volatile bool waiting_player = false;
    volatile bool player_ack = false;
    volatile bool packet_move = false;
    volatile bool packet_ready = false;
    volatile bool invitation_accepted = false;
    volatile bool invitation_requested = false;

    char user_id[7] = {'\0'};
    std::vector<ttt_player> ttt_players;
    ttt_packet tttp;
    ttt_player * selected_player = NULL;
    void (*transmit_list_add_callback)(lora_packet);
    pthread_mutex_t * lvgl_mutex;

    bool checkVictory();
    bool checkDraw();
    void initUI();
    void init_board();
    void simulate_click(uint8_t row, uint8_t col);
    tictactoe(lv_obj_t * parent, void (*transmit_list_add_callback)(lora_packet), pthread_mutex_t * lvgl_mutex);
    //~tictactoe();
    bool add_player(ttt_player p);
    bool del_player(ttt_player p);
    void refresh_players_list();
    void process_packet(ttt_packet p);
    ttt_player * get_player_by_id(const char * id);
    void showUI();
    void show();
    void hideUI();
};
