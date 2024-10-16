#pragma once
#include <Arduino.h>
#include <lvgl.h>

#define APP_TICTACTOE 4

class tictactoe{
    private:
        lv_obj_t * mainFrm;
        lv_obj_t * frame;
        lv_obj_t * btns[3][3];
        char board[3][3];
        char player = 'X';
        bool active = true;

        
    public:

};