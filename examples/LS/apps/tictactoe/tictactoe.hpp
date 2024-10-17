#pragma once
#include <Arduino.h>
#include <lvgl.h>

#define APP_TICTACTOE 4

class tictactoe{
    private:
        
    public:
    // UI objects
    lv_obj_t * parent;
    lv_obj_t * frm_tictactoe;
    lv_obj_t * frm_tictactoe_btn_title;
    lv_obj_t * frm_tictactoe_btn_title_lbl;
    lv_obj_t * frm_tictactoe_btn_back;
    lv_obj_t * frm_tictactoe_btn_back_lbl;
    lv_obj_t * frm_tictactoe_board;
    lv_obj_t * btns[3][3];
    
    bool checkVictory();
    bool checkDraw();
    char board[3][3];
    char player = 'X';
    bool active = true;
    void initUI(lv_obj_t * parent);
    tictactoe(lv_obj_t * parent);
    ~tictactoe();
};