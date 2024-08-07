#include "lcsExpr.h"

#ifndef _WIN32
#include <string>
#include <dlfcn.h>
#include <stddef.h>
#include <iostream>
typedef void *HINSTANCE;
#define GetProcAddress dlsym
//#include <qcoreapplication.h>
#else
#include <Windows.h>
#include <cstdio>
#endif

volatile HINSTANCE gLibLCS = NULL;

LCS_INIT_DLL lcs_init_dll;
LCS_FREE_DLL lcs_free_dll;
LCS_GET_LAST_ERROR lcs_get_last_error;
LCS_N_GET_LAST_ERROR lcs_n_get_last_error;
LCS_COUNT_CARDS lcs_count_cards;
LCS_SEARCH_CARDS lcs_search_cards;
LCS_GET_SERIAL_NUMBER lcs_get_serial_number;
LCS_N_GET_SERIAL_NUMBER lcs_n_get_serial_number;
LCS_ASSIGN_CARD lcs_assign_card;
LCS_REMOVE_CARD lcs_remove_card;
LCS_ACQUIRE_CARD lcs_acquire_card;
LCS_RELEASE_CARD lcs_release_card;
LCS_SELECT_CARD lcs_select_card;
//LCS_SET_START_LIST_POS lcs_set_start_list_pos;
//LCS_N_SET_START_LIST_POS lcs_n_set_start_list_pos;
LCS_SET_START_LIST lcs_set_start_list;
LCS_N_SET_START_LIST lcs_n_set_start_list;
//LCS_SET_START_LIST_1 lcs_set_start_list_1;
//LCS_N_SET_START_LIST_1 lcs_n_set_start_list_1;
//LCS_SET_START_LIST_2 lcs_set_start_list_2;
//LCS_N_SET_START_LIST_2 lcs_n_set_start_list_2;
//LCS_SET_INPUT_POINTER lcs_set_input_pointer;
//LCS_N_SET_INPUT_POINTER lcs_n_set_input_pointer;
LCS_LOAD_LIST lcs_load_list;
LCS_N_LOAD_LIST lcs_n_load_list;
LCS_LOAD_SUB lcs_load_sub;
LCS_N_LOAD_SUB lcs_n_load_sub;
LCS_SUB_CALL lcs_sub_call;
LCS_N_SUB_CALL lcs_n_sub_call;
LCS_LOAD_CHAR lcs_load_char;
LCS_N_LOAD_CHAR lcs_n_load_char;
LCS_MARK_TEXT lcs_mark_text;
LCS_N_MARK_TEXT lcs_n_mark_text;
LCS_MARK_TEXT_ABS lcs_mark_text_abs;
LCS_N_MARK_TEXT_ABS lcs_n_mark_text_abs;
LCS_MARK_CHAR lcs_mark_char;
LCS_N_MARK_CHAR lcs_n_mark_char;
LCS_MARK_CHAR_ABS lcs_mark_char_abs;
LCS_N_MARK_CHAR_ABS lcs_n_mark_char_abs;
LCS_GET_LIST_POINTER lcs_get_list_pointer;
LCS_N_GET_LIST_POINTER lcs_n_get_list_pointer;
LCS_GET_INPUT_POINTER lcs_get_input_pointer;
LCS_N_GET_INPUT_POINTER lcs_n_get_input_pointer;
LCS_EXECUTE_LIST_POS lcs_execute_list_pos;
LCS_N_EXECUTE_LIST_POS lcs_n_execute_list_pos;
LCS_EXECUTE_AT_POINTER lcs_execute_at_pointer;
LCS_N_EXECUTE_AT_POINTER lcs_n_execute_at_pointer;
LCS_EXECUTE_LIST lcs_execute_list;
LCS_N_EXECUTE_LIST lcs_n_execute_list;
//lcs_execute_list_1 lcs_execute_list_1;
//lcs_n_execute_list_1 lcs_n_execute_list_1;
//LCS_EXECUTE_LIST_2 lcs_execute_list_2;
//LCS_N_EXECUTE_LIST_2 lcs_n_execute_list_2;
LCS_AUTO_CHANGE lcs_auto_change;
LCS_N_AUTO_CHANGE lcs_n_auto_change;
LCS_AUTO_CHANGE_POS lcs_auto_change_pos;
LCS_N_AUTO_CHANGE_POS lcs_n_auto_change_pos;
LCS_START_LOOP lcs_start_loop;
LCS_N_START_LOOP lcs_n_start_loop;
LCS_QUIT_LOOP lcs_quit_loop;
LCS_N_QUIT_LOOP lcs_n_quit_loop;
LCS_PAUSE_LIST lcs_pause_list;
LCS_N_PAUSE_LIST lcs_n_pause_list;
LCS_RESTART_LIST lcs_restart_list;
LCS_N_RESTART_LIST lcs_n_restart_list;
LCS_STOP_EXECUTION lcs_stop_execution;
LCS_N_STOP_EXECUTION lcs_n_stop_execution;
//LCS_STOP_LIST lcs_stop_list;
//LCS_N_STOP_LIST lcs_n_stop_list;
LCS_READ_STATUS lcs_read_status;
LCS_N_READ_STATUS lcs_n_read_status;
LCS_GET_STATUS lcs_get_status;
LCS_N_GET_STATUS lcs_n_get_status;
LCS_WRITE_IO_PORT_MASK lcs_write_io_port_mask;
LCS_N_WRITE_IO_PORT_MASK lcs_n_write_io_port_mask;
LCS_READ_IO_PORT lcs_read_io_port;
LCS_N_READ_IO_PORT lcs_n_read_io_port;
LCS_GET_IO_STATUS lcs_get_io_status;
LCS_N_GET_IO_STATUS lcs_n_get_io_status;
LCS_WRITE_DA_X lcs_write_da_x;
LCS_N_WRITE_DA_X lcs_n_write_da_x;
//LCS_WRITE_DA_1 lcs_write_da_1;
//LCS_N_WRITE_DA_1 lcs_n_write_da_1;
//LCS_WRITE_DA_2 lcs_write_da_2;
//LCS_N_WRITE_DA_2 lcs_n_write_da_2;
LCS_WRITE_IO_PORT lcs_write_io_port;
LCS_N_WRITE_IO_PORT lcs_n_write_io_port;
LCS_DISABLE_LASER lcs_disable_laser;
LCS_N_DISABLE_LASER lcs_n_disable_laser;
LCS_ENABLE_LASER lcs_enable_laser;
LCS_N_ENABLE_LASER lcs_n_enable_laser;
LCS_SET_STANDBY lcs_set_standby;
LCS_N_SET_STANDBY lcs_n_set_standby;
LCS_GET_STANDBY lcs_get_standby;
LCS_N_GET_STANDBY lcs_n_get_standby;
LCS_SET_LASER_PULSES_CTRL lcs_set_laser_pulses_ctrl;
LCS_N_SET_LASER_PULSES_CTRL lcs_n_set_laser_pulses_ctrl;
LCS_SET_FIRSTPULSE_KILLER lcs_set_firstpulse_killer;
LCS_N_SET_FIRSTPULSE_KILLER lcs_n_set_firstpulse_killer;
LCS_SET_QSWITCH_DELAY lcs_set_qswitch_delay;
LCS_N_SET_QSWITCH_DELAY lcs_n_set_qswitch_delay;
LCS_SET_LASER_MODE lcs_set_laser_mode;
LCS_N_SET_LASER_MODE lcs_n_set_laser_mode;
LCS_SET_LASER_CONTROL lcs_set_laser_control;
LCS_N_SET_LASER_CONTROL lcs_n_set_laser_control;
LCS_HOME_POSITION lcs_home_position;
LCS_N_HOME_POSITION lcs_n_home_position;
LCS_GOTO_XY lcs_goto_xy;
LCS_N_GOTO_XY lcs_n_goto_xy;
LCS_SET_OFFSET lcs_set_offset;
LCS_N_SET_OFFSET lcs_n_set_offset;
LCS_SET_ANGLE lcs_set_angle;
LCS_N_SET_ANGLE lcs_n_set_angle;
LCS_SET_DELAY_MODE lcs_set_delay_mode;
LCS_N_SET_DELAY_MODE lcs_n_set_delay_mode;
LCS_SET_JUMP_SPEED_CTRL lcs_set_jump_speed_ctrl;
LCS_N_SET_JUMP_SPEED_CTRL lcs_n_set_jump_speed_ctrl;
LCS_SET_MARK_SPEED_CTRL lcs_set_mark_speed_ctrl;
LCS_N_SET_MARK_SPEED_CTRL lcs_n_set_mark_speed_ctrl;
LCS_SELECT_COR_TABLE_LIST lcs_select_cor_table_list;
LCS_N_SELECT_COR_TABLE_LIST lcs_n_select_cor_table_list;
//LCS_LIST_NOP lcs_list_nop;
//LCS_N_LIST_NOP lcs_n_list_nop;
//LCS_LIST_CONTINUE lcs_list_continue;
//LCS_N_LIST_CONTINUE lcs_n_list_continue;
LCS_LONG_DELAY lcs_long_delay;
LCS_N_LONG_DELAY lcs_n_long_delay;
LCS_SET_END_OF_LIST lcs_set_end_of_list;
LCS_N_SET_END_OF_LIST lcs_n_set_end_of_list;
LCS_LIST_JUMP_POS lcs_list_jump_pos;
LCS_N_LIST_JUMP_POS lcs_n_list_jump_pos;
LCS_LIST_JUMP_REL lcs_list_jump_rel;
LCS_N_LIST_JUMP_REL lcs_n_list_jump_rel;
LCS_LIST_REPEAT lcs_list_repeat;
LCS_N_LIST_REPEAT lcs_n_list_repeat;
LCS_LIST_UNTIL lcs_list_until;
LCS_N_LIST_UNTIL lcs_n_list_until;
//LCS_SET_LIST_JUMP lcs_set_list_jump;
//LCS_N_SET_LIST_JUMP lcs_n_set_list_jump;
LCS_LIST_RETURN lcs_list_return;
LCS_N_LIST_RETURN lcs_n_list_return;
LCS_WRITE_IO_PORT_MASK_LIST lcs_write_io_port_mask_list;
LCS_N_WRITE_IO_PORT_MASK_LIST lcs_n_write_io_port_mask_list;
LCS_WRITE_IO_PORT_LIST lcs_write_io_port_list;
LCS_N_WRITE_IO_PORT_LIST lcs_n_write_io_port_list;
//LCS_WRITE_DA_1_LIST lcs_write_da_1_list;
//LCS_N_WRITE_DA_1_LIST lcs_n_write_da_1_list;
//LCS_WRITE_DA_2_LIST lcs_write_da_2_list;
//LCS_N_WRITE_DA_2_LIST lcs_n_write_da_2_list;
LCS_WRITE_DA_X_LIST lcs_write_da_x_list;
LCS_N_WRITE_DA_X_LIST lcs_n_write_da_x_list;
LCS_LASER_ON_LIST lcs_laser_on_list;
LCS_N_LASER_ON_LIST lcs_n_laser_on_list;
LCS_SET_LASER_DELAYS lcs_set_laser_delays;
LCS_N_SET_LASER_DELAYS lcs_n_set_laser_delays;
LCS_SET_STANDBY_LIST lcs_set_standby_list;
LCS_N_SET_STANDBY_LIST lcs_n_set_standby_list;
LCS_SET_LASER_PULSES lcs_set_laser_pulses;
LCS_N_SET_LASER_PULSES lcs_n_set_laser_pulses;
LCS_SET_FIRSTPULSE_KILLER_LIST lcs_set_firstpulse_killer_list;
LCS_N_SET_FIRSTPULSE_KILLER_LIST lcs_n_set_firstpulse_killer_list;
LCS_SET_QSWITCH_DELAY_LIST lcs_set_qswitch_delay_list;
LCS_N_SET_QSWITCH_DELAY_LIST lcs_n_set_qswitch_delay_list;
//LCS_SET_WOBBLE lcs_set_wobble;
//LCS_N_SET_WOBBLE lcs_n_set_wobble;
LCS_SET_WOBBLE_MODE lcs_set_wobble_mode;
LCS_N_SET_WOBBLE_MODE lcs_n_set_wobble_mode;
LCS_MARK_ABS lcs_mark_abs;
LCS_N_MARK_ABS lcs_n_mark_abs;
LCS_MARK_REL lcs_mark_rel;
LCS_N_MARK_REL lcs_n_mark_rel;
LCS_JUMP_ABS lcs_jump_abs;
LCS_N_JUMP_ABS lcs_n_jump_abs;
LCS_JUMP_REL lcs_jump_rel;
LCS_N_JUMP_REL lcs_n_jump_rel;
LCS_SET_OFFSET_LIST lcs_set_offset_list;
LCS_N_SET_OFFSET_LIST lcs_n_set_offset_list;
LCS_SET_ANGLE_LIST lcs_set_angle_list;
LCS_N_SET_ANGLE_LIST lcs_n_set_angle_list;
LCS_SET_MARK_SPEED lcs_set_mark_speed;
LCS_N_SET_MARK_SPEED lcs_n_set_mark_speed;
LCS_SET_JUMP_SPEED lcs_set_jump_speed;
LCS_N_SET_JUMP_SPEED lcs_n_set_jump_speed;
LCS_SET_SCANNER_DELAYS lcs_set_scanner_delays;
LCS_N_SET_SCANNER_DELAYS lcs_n_set_scanner_delays;
LCS_LOAD_CORRECTION_FILE lcs_load_correction_file;
LCS_N_LOAD_CORRECTION_FILE lcs_n_load_correction_file;
LCS_SELECT_COR_TABLE lcs_select_cor_table;
LCS_N_SELECT_COR_TABLE lcs_n_select_cor_table;
LCS_SET_MANUAL_CORRECTION_PARAMS lcs_set_manual_correction_params;
LCS_N_SET_MANUAL_CORRECTION_PARAMS lcs_n_set_manual_correction_params;
LCS_SET_SCANAHEAD_PARAMS lcs_set_scanahead_params;
LCS_N_SET_SCANAHEAD_PARAMS lcs_n_set_scanahead_params;
LCS_CLEAR_MARK_COUNT lcs_clear_mark_count;
LCS_N_CLEAR_MARK_COUNT lcs_n_clear_mark_count;
LCS_GET_MARK_COUNT lcs_get_mark_count;
LCS_N_GET_MARK_COUNT lcs_n_get_mark_count;
LCS_INC_MARK_COUNT lcs_inc_mark_count;
LCS_N_INC_MARK_COUNT lcs_n_inc_mark_count;
LCS_SET_LASER_POWER lcs_set_laser_power;
LCS_N_SET_LASER_POWER lcs_n_set_laser_power;
//LCS_SET_AXIS_PARA lcs_set_axis_para;
//LCS_N_SET_AXIS_PARA lcs_n_set_axis_para;
//LCS_SET_AXIS_SPEED lcs_set_axis_speed;
//LCS_N_SET_AXIS_SPEED lcs_n_set_axis_speed;
//LCS_SET_AXIS_ZERO_SPEED lcs_set_axis_zero_speed;
//LCS_N_SET_AXIS_ZERO_SPEED lcs_n_set_axis_zero_speed;
LCS_SET_AXIS_MOVE lcs_set_axis_move;
LCS_N_SET_AXIS_MOVE lcs_n_set_axis_move;
LCS_SET_AXIS_TOZERO lcs_set_axis_tozero;
LCS_N_SET_AXIS_TOZERO lcs_n_set_axis_tozero;
LCS_GET_AXIS_STATUS lcs_get_axis_status;
LCS_N_GET_AXIS_STATUS lcs_n_get_axis_status;
LCS_ETH_SET_SEARCH_CARDS_TIMEOUT lcs_eth_set_search_cards_timeout;
LCS_ETH_COUNT_CARDS lcs_eth_count_cards;
LCS_ETH_SEARCH_CARDS lcs_eth_search_cards;
LCS_ETH_SEARCH_CARDS_RANGE lcs_eth_search_cards_range;
LCS_ETH_GET_IP lcs_eth_get_ip;
LCS_ETH_GET_IP_SEARCH lcs_eth_get_ip_search;
LCS_ETH_SET_STATIC_IP lcs_eth_set_static_ip;
LCS_N_ETH_SET_STATIC_IP lcs_n_eth_set_static_ip;
LCS_ETH_GET_STATIC_IP lcs_eth_get_static_ip;
LCS_N_ETH_GET_STATIC_IP lcs_n_eth_get_static_ip;
LCS_ETH_ASSIGN_CARD_IP lcs_eth_assign_card_ip;
LCS_ETH_ASSIGN_CARD lcs_eth_assign_card;
LCS_ETH_REMOVE_CARD lcs_eth_remove_card;


//  LCS2open
//
//  This function explicitly or dynamically links to the lcs2dll.DLL or lcs2dll.so.
//  Call it before using any LCS2 function.
//
//      Return      Meaning
//
//       0          Success. Using of the LCS2 functions is possible.
//      -1          Error: file lcs2dll.DLL or lcs2dll.so not found. The LCS2 functions cannot be used.
//      -2          Error: file lcs2dll.DLL or lcs2dll.so is already loaded.
long LCS2open(void) {
    if (gLibLCS)
        return (-2);

#ifdef _WIN32
#if !defined(_WIN64)
    gLibLCS = LoadLibraryA("lcs2dll.DLL");
#else
    gLibLCS = LoadLibraryA("lcs2dllx64.DLL");
#endif // !defined(_WIN64)
#elif defined(__APPLE__)
    std::string sLibPath =  "/Users/simon/Dev/bsl-sdk/mac/Debug/liblcs2dll.dylib";
    std::cout << "Current LibPath" << sLibPath << "\n";
    gLibLCS = dlopen(sLibPath.c_str(), RTLD_NOW | RTLD_LOCAL);

    if (!gLibLCS) {
        std::string sErr = dlerror();
        std::cout << "Unable to link libLCS %s", sErr.c_str();
        sErr = sErr;
    }

#else
    gLibLCS = dlopen("liblcs2dll.so",  RTLD_NOW | RTLD_LOCAL);

    if (!gLibLCS) {
        std::string sErr = dlerror();
        sErr = sErr;
    }

#endif

    if (!gLibLCS)
        return (-1);

    lcs_init_dll = (LCS_INIT_DLL)GetProcAddress(gLibLCS, "init_dll");
    lcs_free_dll = (LCS_FREE_DLL)GetProcAddress(gLibLCS, "free_dll");
    lcs_get_last_error = (LCS_GET_LAST_ERROR)GetProcAddress(gLibLCS, "get_last_error");
    lcs_n_get_last_error = (LCS_N_GET_LAST_ERROR)GetProcAddress(gLibLCS, "n_get_last_error");
    lcs_count_cards = (LCS_COUNT_CARDS)GetProcAddress(gLibLCS, "count_cards");
    lcs_search_cards = (LCS_SEARCH_CARDS)GetProcAddress(gLibLCS, "search_cards");
    lcs_get_serial_number = (LCS_GET_SERIAL_NUMBER)GetProcAddress(gLibLCS, "get_serial_number");
    lcs_n_get_serial_number = (LCS_N_GET_SERIAL_NUMBER)GetProcAddress(gLibLCS, "n_get_serial_number");
    lcs_assign_card = (LCS_ASSIGN_CARD)GetProcAddress(gLibLCS, "assign_card");
    lcs_remove_card = (LCS_REMOVE_CARD)GetProcAddress(gLibLCS, "remove_card");
    lcs_acquire_card = (LCS_ACQUIRE_CARD)GetProcAddress(gLibLCS, "acquire_card");
    lcs_release_card = (LCS_RELEASE_CARD)GetProcAddress(gLibLCS, "release_card");
    lcs_select_card = (LCS_SELECT_CARD)GetProcAddress(gLibLCS, "select_card");
    //	 lcs_set_start_list_pos = (LCS_SET_START_LIST_POS)GetProcAddress(gLibLCS, "set_start_list_pos");
    //	 lcs_n_set_start_list_pos = (LCS_N_SET_START_LIST_POS)GetProcAddress(gLibLCS, "n_set_start_list_pos");
    lcs_set_start_list = (LCS_SET_START_LIST)GetProcAddress(gLibLCS, "set_start_list");
    lcs_n_set_start_list = (LCS_N_SET_START_LIST)GetProcAddress(gLibLCS, "n_set_start_list");
    //	 lcs_set_start_list_1 = (LCS_SET_START_LIST_1)GetProcAddress(gLibLCS, "set_start_list_1");
    //	 lcs_n_set_start_list_1 = (LCS_N_SET_START_LIST_1)GetProcAddress(gLibLCS, "n_set_start_list_1");
    //	 lcs_set_start_list_2 = (LCS_SET_START_LIST_2)GetProcAddress(gLibLCS, "set_start_list_2");
    //	 lcs_n_set_start_list_2 = (LCS_N_SET_START_LIST_2)GetProcAddress(gLibLCS, "n_set_start_list_2");
    //	 lcs_set_input_pointer = (LCS_SET_INPUT_POINTER)GetProcAddress(gLibLCS, "set_input_pointer");
    //	 lcs_n_set_input_pointer = (LCS_N_SET_INPUT_POINTER)GetProcAddress(gLibLCS, "n_set_input_pointer");
    lcs_load_list = (LCS_LOAD_LIST)GetProcAddress(gLibLCS, "load_list");
    lcs_n_load_list = (LCS_N_LOAD_LIST)GetProcAddress(gLibLCS, "n_load_list");
    lcs_load_sub = (LCS_LOAD_SUB)GetProcAddress(gLibLCS, "load_sub");
    lcs_n_load_sub = (LCS_N_LOAD_SUB)GetProcAddress(gLibLCS, "n_load_sub");
    lcs_sub_call = (LCS_SUB_CALL)GetProcAddress(gLibLCS, "sub_call");
    lcs_n_sub_call = (LCS_N_SUB_CALL)GetProcAddress(gLibLCS, "n_sub_call");
    lcs_load_char = (LCS_LOAD_CHAR)GetProcAddress(gLibLCS, "load_char");
    lcs_n_load_char = (LCS_N_LOAD_CHAR)GetProcAddress(gLibLCS, "n_load_char");
    lcs_mark_text = (LCS_MARK_TEXT)GetProcAddress(gLibLCS, "mark_text");
    lcs_n_mark_text = (LCS_N_MARK_TEXT)GetProcAddress(gLibLCS, "n_mark_text");
    lcs_mark_text_abs = (LCS_MARK_TEXT_ABS)GetProcAddress(gLibLCS, "mark_text_abs");
    lcs_n_mark_text_abs = (LCS_N_MARK_TEXT_ABS)GetProcAddress(gLibLCS, "n_mark_text_abs");
    lcs_mark_char = (LCS_MARK_CHAR)GetProcAddress(gLibLCS, "mark_char");
    lcs_n_mark_char = (LCS_N_MARK_CHAR)GetProcAddress(gLibLCS, "n_mark_char");
    lcs_mark_char_abs = (LCS_MARK_CHAR_ABS)GetProcAddress(gLibLCS, "mark_char_abs");
    lcs_n_mark_char_abs = (LCS_N_MARK_CHAR_ABS)GetProcAddress(gLibLCS, "n_mark_char_abs");
    lcs_get_list_pointer = (LCS_GET_LIST_POINTER)GetProcAddress(gLibLCS, "get_list_pointer");
    lcs_n_get_list_pointer = (LCS_N_GET_LIST_POINTER)GetProcAddress(gLibLCS, "n_get_list_pointer");
    lcs_get_input_pointer = (LCS_GET_INPUT_POINTER)GetProcAddress(gLibLCS, "get_input_pointer");
    lcs_n_get_input_pointer = (LCS_N_GET_INPUT_POINTER)GetProcAddress(gLibLCS, "n_get_input_pointer");
    lcs_execute_list_pos = (LCS_EXECUTE_LIST_POS)GetProcAddress(gLibLCS, "execute_list_pos");
    lcs_n_execute_list_pos = (LCS_N_EXECUTE_LIST_POS)GetProcAddress(gLibLCS, "n_execute_list_pos");
    lcs_execute_at_pointer = (LCS_EXECUTE_AT_POINTER)GetProcAddress(gLibLCS, "execute_at_pointer");
    lcs_n_execute_at_pointer = (LCS_N_EXECUTE_AT_POINTER)GetProcAddress(gLibLCS, "n_execute_at_pointer");
    lcs_execute_list = (LCS_EXECUTE_LIST)GetProcAddress(gLibLCS, "execute_list");
    lcs_n_execute_list = (LCS_N_EXECUTE_LIST)GetProcAddress(gLibLCS, "n_execute_list");
    //	 lcs_execute_list_1 = (LCS_EXECUTE_LIST_1)GetProcAddress(gLibLCS, "execute_list_1");
    //	 lcs_n_execute_list_1 = (LCS_N_EXECUTE_LIST_1)GetProcAddress(gLibLCS, "n_execute_list_1");
    //	 lcs_execute_list_2 = (LCS_EXECUTE_LIST_2)GetProcAddress(gLibLCS, "execute_list_2");
    //	 lcs_n_execute_list_2 = (LCS_N_EXECUTE_LIST_2)GetProcAddress(gLibLCS, "n_execute_list_2");
    lcs_auto_change = (LCS_AUTO_CHANGE)GetProcAddress(gLibLCS, "auto_change");
    lcs_n_auto_change = (LCS_N_AUTO_CHANGE)GetProcAddress(gLibLCS, "n_auto_change");
    lcs_auto_change_pos = (LCS_AUTO_CHANGE_POS)GetProcAddress(gLibLCS, "auto_change_pos");
    lcs_n_auto_change_pos = (LCS_N_AUTO_CHANGE_POS)GetProcAddress(gLibLCS, "n_auto_change_pos");
    lcs_start_loop = (LCS_START_LOOP)GetProcAddress(gLibLCS, "start_loop");
    lcs_n_start_loop = (LCS_N_START_LOOP)GetProcAddress(gLibLCS, "n_start_loop");
    lcs_quit_loop = (LCS_QUIT_LOOP)GetProcAddress(gLibLCS, "quit_loop");
    lcs_n_quit_loop = (LCS_N_QUIT_LOOP)GetProcAddress(gLibLCS, "n_quit_loop");
    lcs_pause_list = (LCS_PAUSE_LIST)GetProcAddress(gLibLCS, "pause_list");
    lcs_n_pause_list = (LCS_N_PAUSE_LIST)GetProcAddress(gLibLCS, "n_pause_list");
    lcs_restart_list = (LCS_RESTART_LIST)GetProcAddress(gLibLCS, "restart_list");
    lcs_n_restart_list = (LCS_N_RESTART_LIST)GetProcAddress(gLibLCS, "n_restart_list");
    lcs_stop_execution = (LCS_STOP_EXECUTION)GetProcAddress(gLibLCS, "stop_execution");
    lcs_n_stop_execution = (LCS_N_STOP_EXECUTION)GetProcAddress(gLibLCS, "n_stop_execution");
    //	 lcs_stop_list = (LCS_STOP_LIST)GetProcAddress(gLibLCS, "stop_list");
    //	 lcs_n_stop_list = (LCS_N_STOP_LIST)GetProcAddress(gLibLCS, "n_stop_list");
    lcs_read_status = (LCS_READ_STATUS)GetProcAddress(gLibLCS, "read_status");
    lcs_n_read_status = (LCS_N_READ_STATUS)GetProcAddress(gLibLCS, "n_read_status");
    lcs_get_status = (LCS_GET_STATUS)GetProcAddress(gLibLCS, "get_status");
    lcs_n_get_status = (LCS_N_GET_STATUS)GetProcAddress(gLibLCS, "n_get_status");
    lcs_write_io_port_mask = (LCS_WRITE_IO_PORT_MASK)GetProcAddress(gLibLCS, "write_io_port_mask");
    lcs_n_write_io_port_mask = (LCS_N_WRITE_IO_PORT_MASK)GetProcAddress(gLibLCS, "n_write_io_port_mask");
    lcs_read_io_port = (LCS_READ_IO_PORT)GetProcAddress(gLibLCS, "read_io_port");
    lcs_n_read_io_port = (LCS_N_READ_IO_PORT)GetProcAddress(gLibLCS, "n_read_io_port");
    lcs_get_io_status = (LCS_GET_IO_STATUS)GetProcAddress(gLibLCS, "get_io_status");
    lcs_n_get_io_status = (LCS_N_GET_IO_STATUS)GetProcAddress(gLibLCS, "n_get_io_status");
    lcs_write_da_x = (LCS_WRITE_DA_X)GetProcAddress(gLibLCS, "write_da_x");
    lcs_n_write_da_x = (LCS_N_WRITE_DA_X)GetProcAddress(gLibLCS, "n_write_da_x");
    //	 lcs_write_da_1 = (LCS_WRITE_DA_1)GetProcAddress(gLibLCS, "write_da_1");
    //	 lcs_n_write_da_1 = (LCS_N_WRITE_DA_1)GetProcAddress(gLibLCS, "n_write_da_1");
    //	 lcs_write_da_2 = (LCS_WRITE_DA_2)GetProcAddress(gLibLCS, "write_da_2");
    //	 lcs_n_write_da_2 = (LCS_N_WRITE_DA_2)GetProcAddress(gLibLCS, "n_write_da_2");
    lcs_write_io_port = (LCS_WRITE_IO_PORT)GetProcAddress(gLibLCS, "write_io_port");
    lcs_n_write_io_port = (LCS_N_WRITE_IO_PORT)GetProcAddress(gLibLCS, "n_write_io_port");
    lcs_disable_laser = (LCS_DISABLE_LASER)GetProcAddress(gLibLCS, "disable_laser");
    lcs_n_disable_laser = (LCS_N_DISABLE_LASER)GetProcAddress(gLibLCS, "n_disable_laser");
    lcs_enable_laser = (LCS_ENABLE_LASER)GetProcAddress(gLibLCS, "enable_laser");
    lcs_n_enable_laser = (LCS_N_ENABLE_LASER)GetProcAddress(gLibLCS, "n_enable_laser");
    lcs_set_standby = (LCS_SET_STANDBY)GetProcAddress(gLibLCS, "set_standby");
    lcs_n_set_standby = (LCS_N_SET_STANDBY)GetProcAddress(gLibLCS, "n_set_standby");
    lcs_get_standby = (LCS_GET_STANDBY)GetProcAddress(gLibLCS, "get_standby");
    lcs_n_get_standby = (LCS_N_GET_STANDBY)GetProcAddress(gLibLCS, "n_get_standby");
    lcs_set_laser_pulses_ctrl = (LCS_SET_LASER_PULSES_CTRL)GetProcAddress(gLibLCS, "set_laser_pulses_ctrl");
    lcs_n_set_laser_pulses_ctrl = (LCS_N_SET_LASER_PULSES_CTRL)GetProcAddress(gLibLCS, "n_set_laser_pulses_ctrl");
    lcs_set_firstpulse_killer = (LCS_SET_FIRSTPULSE_KILLER)GetProcAddress(gLibLCS, "set_firstpulse_killer");
    lcs_n_set_firstpulse_killer = (LCS_N_SET_FIRSTPULSE_KILLER)GetProcAddress(gLibLCS, "n_set_firstpulse_killer");
    lcs_set_qswitch_delay = (LCS_SET_QSWITCH_DELAY)GetProcAddress(gLibLCS, "set_qswitch_delay");
    lcs_n_set_qswitch_delay = (LCS_N_SET_QSWITCH_DELAY)GetProcAddress(gLibLCS, "n_set_qswitch_delay");
    lcs_set_laser_mode = (LCS_SET_LASER_MODE)GetProcAddress(gLibLCS, "set_laser_mode");
    lcs_n_set_laser_mode = (LCS_N_SET_LASER_MODE)GetProcAddress(gLibLCS, "n_set_laser_mode");
    lcs_set_laser_control = (LCS_SET_LASER_CONTROL)GetProcAddress(gLibLCS, "set_laser_control");
    lcs_n_set_laser_control = (LCS_N_SET_LASER_CONTROL)GetProcAddress(gLibLCS, "n_set_laser_control");
    lcs_home_position = (LCS_HOME_POSITION)GetProcAddress(gLibLCS, "home_position");
    lcs_n_home_position = (LCS_N_HOME_POSITION)GetProcAddress(gLibLCS, "n_home_position");
    lcs_goto_xy = (LCS_GOTO_XY)GetProcAddress(gLibLCS, "goto_xy");
    lcs_n_goto_xy = (LCS_N_GOTO_XY)GetProcAddress(gLibLCS, "n_goto_xy");
    lcs_set_offset = (LCS_SET_OFFSET)GetProcAddress(gLibLCS, "set_offset");
    lcs_n_set_offset = (LCS_N_SET_OFFSET)GetProcAddress(gLibLCS, "n_set_offset");
    lcs_set_angle = (LCS_SET_ANGLE)GetProcAddress(gLibLCS, "set_angle");
    lcs_n_set_angle = (LCS_N_SET_ANGLE)GetProcAddress(gLibLCS, "n_set_angle");
    lcs_set_delay_mode = (LCS_SET_DELAY_MODE)GetProcAddress(gLibLCS, "set_delay_mode");
    lcs_n_set_delay_mode = (LCS_N_SET_DELAY_MODE)GetProcAddress(gLibLCS, "n_set_delay_mode");
    lcs_set_jump_speed_ctrl = (LCS_SET_JUMP_SPEED_CTRL)GetProcAddress(gLibLCS, "set_jump_speed_ctrl");
    lcs_n_set_jump_speed_ctrl = (LCS_N_SET_JUMP_SPEED_CTRL)GetProcAddress(gLibLCS, "n_set_jump_speed_ctrl");
    lcs_set_mark_speed_ctrl = (LCS_SET_MARK_SPEED_CTRL)GetProcAddress(gLibLCS, "set_mark_speed_ctrl");
    lcs_n_set_mark_speed_ctrl = (LCS_N_SET_MARK_SPEED_CTRL)GetProcAddress(gLibLCS, "n_set_mark_speed_ctrl");
    lcs_select_cor_table_list = (LCS_SELECT_COR_TABLE_LIST)GetProcAddress(gLibLCS, "select_cor_table_list");
    lcs_n_select_cor_table_list = (LCS_N_SELECT_COR_TABLE_LIST)GetProcAddress(gLibLCS, "n_select_cor_table_list");
    //	 lcs_list_nop = (LCS_LIST_NOP)GetProcAddress(gLibLCS, "list_nop");
    //	 lcs_n_list_nop = (LCS_N_LIST_NOP)GetProcAddress(gLibLCS, "n_list_nop");
    //	 lcs_list_continue = (LCS_LIST_CONTINUE)GetProcAddress(gLibLCS, "list_continue");
    //	 lcs_n_list_continue = (LCS_N_LIST_CONTINUE)GetProcAddress(gLibLCS, "n_list_continue");
    lcs_long_delay = (LCS_LONG_DELAY)GetProcAddress(gLibLCS, "long_delay");
    lcs_n_long_delay = (LCS_N_LONG_DELAY)GetProcAddress(gLibLCS, "n_long_delay");
    lcs_set_end_of_list = (LCS_SET_END_OF_LIST)GetProcAddress(gLibLCS, "set_end_of_list");
    lcs_n_set_end_of_list = (LCS_N_SET_END_OF_LIST)GetProcAddress(gLibLCS, "n_set_end_of_list");
    lcs_list_jump_pos = (LCS_LIST_JUMP_POS)GetProcAddress(gLibLCS, "list_jump_pos");
    lcs_n_list_jump_pos = (LCS_N_LIST_JUMP_POS)GetProcAddress(gLibLCS, "n_list_jump_pos");
    lcs_list_jump_rel = (LCS_LIST_JUMP_REL)GetProcAddress(gLibLCS, "list_jump_rel");
    lcs_n_list_jump_rel = (LCS_N_LIST_JUMP_REL)GetProcAddress(gLibLCS, "n_list_jump_rel");
    lcs_list_repeat = (LCS_LIST_REPEAT)GetProcAddress(gLibLCS, "list_repeat");
    lcs_n_list_repeat = (LCS_N_LIST_REPEAT)GetProcAddress(gLibLCS, "n_list_repeat");
    lcs_list_until = (LCS_LIST_UNTIL)GetProcAddress(gLibLCS, "list_until");
    lcs_n_list_until = (LCS_N_LIST_UNTIL)GetProcAddress(gLibLCS, "n_list_until");
    //	 lcs_set_list_jump = (LCS_SET_LIST_JUMP)GetProcAddress(gLibLCS, "set_list_jump");
    //	 lcs_n_set_list_jump = (LCS_N_SET_LIST_JUMP)GetProcAddress(gLibLCS, "n_set_list_jump");
    lcs_list_return = (LCS_LIST_RETURN)GetProcAddress(gLibLCS, "list_return");
    lcs_n_list_return = (LCS_N_LIST_RETURN)GetProcAddress(gLibLCS, "n_list_return");
    lcs_write_io_port_mask_list = (LCS_WRITE_IO_PORT_MASK_LIST)GetProcAddress(gLibLCS, "write_io_port_mask_list");
    lcs_n_write_io_port_mask_list = (LCS_N_WRITE_IO_PORT_MASK_LIST)GetProcAddress(gLibLCS, "n_write_io_port_mask_list");
    lcs_write_io_port_list = (LCS_WRITE_IO_PORT_LIST)GetProcAddress(gLibLCS, "write_io_port_list");
    lcs_n_write_io_port_list = (LCS_N_WRITE_IO_PORT_LIST)GetProcAddress(gLibLCS, "n_write_io_port_list");
    //	 lcs_write_da_1_list = (LCS_WRITE_DA_1_LIST)GetProcAddress(gLibLCS, "write_da_1_list");
    //	 lcs_n_write_da_1_list = (LCS_N_WRITE_DA_1_LIST)GetProcAddress(gLibLCS, "n_write_da_1_list");
    //	 lcs_write_da_2_list = (LCS_WRITE_DA_2_LIST)GetProcAddress(gLibLCS, "write_da_2_list");
    //	 lcs_n_write_da_2_list = (LCS_N_WRITE_DA_2_LIST)GetProcAddress(gLibLCS, "n_write_da_2_list");
    lcs_write_da_x_list = (LCS_WRITE_DA_X_LIST)GetProcAddress(gLibLCS, "write_da_x_list");
    lcs_n_write_da_x_list = (LCS_N_WRITE_DA_X_LIST)GetProcAddress(gLibLCS, "n_write_da_x_list");
    lcs_laser_on_list = (LCS_LASER_ON_LIST)GetProcAddress(gLibLCS, "laser_on_list");
    lcs_n_laser_on_list = (LCS_N_LASER_ON_LIST)GetProcAddress(gLibLCS, "n_laser_on_list");
    lcs_set_laser_delays = (LCS_SET_LASER_DELAYS)GetProcAddress(gLibLCS, "set_laser_delays");
    lcs_n_set_laser_delays = (LCS_N_SET_LASER_DELAYS)GetProcAddress(gLibLCS, "n_set_laser_delays");
    lcs_set_standby_list = (LCS_SET_STANDBY_LIST)GetProcAddress(gLibLCS, "set_standby_list");
    lcs_n_set_standby_list = (LCS_N_SET_STANDBY_LIST)GetProcAddress(gLibLCS, "n_set_standby_list");
    lcs_set_laser_pulses = (LCS_SET_LASER_PULSES)GetProcAddress(gLibLCS, "set_laser_pulses");
    lcs_n_set_laser_pulses = (LCS_N_SET_LASER_PULSES)GetProcAddress(gLibLCS, "n_set_laser_pulses");
    lcs_set_firstpulse_killer_list = (LCS_SET_FIRSTPULSE_KILLER_LIST)GetProcAddress(gLibLCS, "set_firstpulse_killer_list");
    lcs_n_set_firstpulse_killer_list = (LCS_N_SET_FIRSTPULSE_KILLER_LIST)GetProcAddress(gLibLCS, "n_set_firstpulse_killer_list");
    lcs_set_qswitch_delay_list = (LCS_SET_QSWITCH_DELAY_LIST)GetProcAddress(gLibLCS, "set_qswitch_delay_list");
    lcs_n_set_qswitch_delay_list = (LCS_N_SET_QSWITCH_DELAY_LIST)GetProcAddress(gLibLCS, "n_set_qswitch_delay_list");
    //	 lcs_set_wobble = (LCS_SET_WOBBLE)GetProcAddress(gLibLCS, "set_wobble");
    //	 lcs_n_set_wobble = (LCS_N_SET_WOBBLE)GetProcAddress(gLibLCS, "n_set_wobble");
    lcs_set_wobble_mode = (LCS_SET_WOBBLE_MODE)GetProcAddress(gLibLCS, "set_wobble_mode");
    lcs_n_set_wobble_mode = (LCS_N_SET_WOBBLE_MODE)GetProcAddress(gLibLCS, "n_set_wobble_mode");
    lcs_mark_abs = (LCS_MARK_ABS)GetProcAddress(gLibLCS, "mark_abs");
    lcs_n_mark_abs = (LCS_N_MARK_ABS)GetProcAddress(gLibLCS, "n_mark_abs");
    lcs_mark_rel = (LCS_MARK_REL)GetProcAddress(gLibLCS, "mark_rel");
    lcs_n_mark_rel = (LCS_N_MARK_REL)GetProcAddress(gLibLCS, "n_mark_rel");
    lcs_jump_abs = (LCS_JUMP_ABS)GetProcAddress(gLibLCS, "jump_abs");
    lcs_n_jump_abs = (LCS_N_JUMP_ABS)GetProcAddress(gLibLCS, "n_jump_abs");
    lcs_jump_rel = (LCS_JUMP_REL)GetProcAddress(gLibLCS, "jump_rel");
    lcs_n_jump_rel = (LCS_N_JUMP_REL)GetProcAddress(gLibLCS, "n_jump_rel");
    lcs_set_offset_list = (LCS_SET_OFFSET_LIST)GetProcAddress(gLibLCS, "set_offset_list");
    lcs_n_set_offset_list = (LCS_N_SET_OFFSET_LIST)GetProcAddress(gLibLCS, "n_set_offset_list");
    lcs_set_angle_list = (LCS_SET_ANGLE_LIST)GetProcAddress(gLibLCS, "set_angle_list");
    lcs_n_set_angle_list = (LCS_N_SET_ANGLE_LIST)GetProcAddress(gLibLCS, "n_set_angle_list");
    lcs_set_mark_speed = (LCS_SET_MARK_SPEED)GetProcAddress(gLibLCS, "set_mark_speed");
    lcs_n_set_mark_speed = (LCS_N_SET_MARK_SPEED)GetProcAddress(gLibLCS, "n_set_mark_speed");
    lcs_set_jump_speed = (LCS_SET_JUMP_SPEED)GetProcAddress(gLibLCS, "set_jump_speed");
    lcs_n_set_jump_speed = (LCS_N_SET_JUMP_SPEED)GetProcAddress(gLibLCS, "n_set_jump_speed");
    lcs_set_scanner_delays = (LCS_SET_SCANNER_DELAYS)GetProcAddress(gLibLCS, "set_scanner_delays");
    lcs_n_set_scanner_delays = (LCS_N_SET_SCANNER_DELAYS)GetProcAddress(gLibLCS, "n_set_scanner_delays");
    lcs_load_correction_file = (LCS_LOAD_CORRECTION_FILE)GetProcAddress(gLibLCS, "load_correction_file");
    lcs_n_load_correction_file = (LCS_N_LOAD_CORRECTION_FILE)GetProcAddress(gLibLCS, "n_load_correction_file");
    lcs_select_cor_table = (LCS_SELECT_COR_TABLE)GetProcAddress(gLibLCS, "select_cor_table");
    lcs_n_select_cor_table = (LCS_N_SELECT_COR_TABLE)GetProcAddress(gLibLCS, "n_select_cor_table");
    lcs_set_manual_correction_params = (LCS_SET_MANUAL_CORRECTION_PARAMS)GetProcAddress(gLibLCS, "set_manual_correction_params");
    lcs_n_set_manual_correction_params = (LCS_N_SET_MANUAL_CORRECTION_PARAMS)GetProcAddress(gLibLCS, "n_set_manual_correction_params");
    lcs_set_scanahead_params = (LCS_SET_SCANAHEAD_PARAMS)GetProcAddress(gLibLCS, "set_scanahead_params");
    lcs_n_set_scanahead_params = (LCS_N_SET_SCANAHEAD_PARAMS)GetProcAddress(gLibLCS, "n_set_scanahead_params");
    lcs_clear_mark_count = (LCS_CLEAR_MARK_COUNT)GetProcAddress(gLibLCS, "clear_mark_count");
    lcs_n_clear_mark_count = (LCS_N_CLEAR_MARK_COUNT)GetProcAddress(gLibLCS, "n_clear_mark_count");
    lcs_get_mark_count = (LCS_GET_MARK_COUNT)GetProcAddress(gLibLCS, "get_mark_count");
    lcs_n_get_mark_count = (LCS_N_GET_MARK_COUNT)GetProcAddress(gLibLCS, "n_get_mark_count");
    lcs_inc_mark_count = (LCS_INC_MARK_COUNT)GetProcAddress(gLibLCS, "inc_mark_count");
    lcs_n_inc_mark_count = (LCS_N_INC_MARK_COUNT)GetProcAddress(gLibLCS, "n_inc_mark_count");
    lcs_set_laser_power = (LCS_SET_LASER_POWER)GetProcAddress(gLibLCS, "set_laser_power");
    lcs_n_set_laser_power = (LCS_N_SET_LASER_POWER)GetProcAddress(gLibLCS, "n_set_laser_power");
    // lcs_set_axis_para = (LCS_SET_AXIS_PARA)GetProcAddress(gLibLCS, "set_axis_para");
    // lcs_n_set_axis_para = (LCS_N_SET_AXIS_PARA)GetProcAddress(gLibLCS, "n_set_axis_para");
    //	 lcs_set_axis_speed = (LCS_SET_AXIS_SPEED)GetProcAddress(gLibLCS, "set_axis_speed");
    //	 lcs_n_set_axis_speed = (LCS_N_SET_AXIS_SPEED)GetProcAddress(gLibLCS, "n_set_axis_speed");
    // lcs_set_axis_zero_speed = (LCS_SET_AXIS_ZERO_SPEED)GetProcAddress(gLibLCS, "set_axis_zero_speed");
    // lcs_n_set_axis_zero_speed = (LCS_N_SET_AXIS_ZERO_SPEED)GetProcAddress(gLibLCS, "n_set_axis_zero_speed");
    lcs_set_axis_move = (LCS_SET_AXIS_MOVE)GetProcAddress(gLibLCS, "set_axis_move");
    lcs_n_set_axis_move = (LCS_N_SET_AXIS_MOVE)GetProcAddress(gLibLCS, "n_set_axis_move");
    lcs_set_axis_tozero = (LCS_SET_AXIS_TOZERO)GetProcAddress(gLibLCS, "set_axis_tozero");
    lcs_n_set_axis_tozero = (LCS_N_SET_AXIS_TOZERO)GetProcAddress(gLibLCS, "n_set_axis_tozero");
    lcs_get_axis_status = (LCS_GET_AXIS_STATUS)GetProcAddress(gLibLCS, "get_axis_status");
    lcs_n_get_axis_status = (LCS_N_GET_AXIS_STATUS)GetProcAddress(gLibLCS, "n_get_axis_status");
    lcs_eth_set_search_cards_timeout = (LCS_ETH_SET_SEARCH_CARDS_TIMEOUT)GetProcAddress(gLibLCS, "eth_set_search_cards_timeout");
    lcs_eth_count_cards = (LCS_ETH_COUNT_CARDS)GetProcAddress(gLibLCS, "eth_count_cards");
    lcs_eth_search_cards = (LCS_ETH_SEARCH_CARDS)GetProcAddress(gLibLCS, "eth_search_cards");
    lcs_eth_search_cards_range = (LCS_ETH_SEARCH_CARDS_RANGE)GetProcAddress(gLibLCS, "eth_search_cards_range");
    lcs_eth_get_ip = (LCS_ETH_GET_IP)GetProcAddress(gLibLCS, "eth_get_ip");
    lcs_eth_get_ip_search = (LCS_ETH_GET_IP_SEARCH)GetProcAddress(gLibLCS, "eth_get_ip_search");
    lcs_eth_set_static_ip = (LCS_ETH_SET_STATIC_IP)GetProcAddress(gLibLCS, "eth_set_static_ip");
    lcs_n_eth_set_static_ip = (LCS_N_ETH_SET_STATIC_IP)GetProcAddress(gLibLCS, "n_eth_set_static_ip");
    lcs_eth_get_static_ip = (LCS_ETH_GET_STATIC_IP)GetProcAddress(gLibLCS, "eth_get_static_ip");
    lcs_n_eth_get_static_ip = (LCS_N_ETH_GET_STATIC_IP)GetProcAddress(gLibLCS, "n_eth_get_static_ip");
    lcs_eth_assign_card_ip = (LCS_ETH_ASSIGN_CARD_IP)GetProcAddress(gLibLCS, "eth_assign_card_ip");
    lcs_eth_assign_card = (LCS_ETH_ASSIGN_CARD)GetProcAddress(gLibLCS, "eth_assign_card");
    lcs_eth_remove_card = (LCS_ETH_REMOVE_CARD)GetProcAddress(gLibLCS, "eth_remove_card");
    return (0);
}

//  LCS2close
//
//  This function terminates the explicit linking to the lcs2dll.DLL or liblcs2.so.
//  Call it when the use of the LCS2 functions is finished.
void LCS2close(void) {
    if (gLibLCS) {
#ifdef _WIN32
        (void)FreeLibrary(gLibLCS);
#else
        (void)dlclose(gLibLCS);
#endif
    }

    if (gLibLCS) {
        gLibLCS = NULL;
    }
}

bool connect_bsl_board() {
    // Note: dylib must be placed in pwd!
    printf("Loading BSL library");
    LCS2open();
    printf("LCSOpen OK");
    if (lcs_init_dll() != LCS_RES_NO_ERROR) {
        printf("Failed to initialize LCS library.");
        return false;
    }

    printf("LCS Init OK");

    if (lcs_search_cards() == 0) {
        printf("No laser cards found.");
        return false;
    }

    if (lcs_select_card(0) != LCS_RES_NO_ERROR) {
        printf("Failed to select laser card.");
        return false;
    }
    return true;
}