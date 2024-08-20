// lcsdll Explicitly linked header files
#include "public.h"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _WIN32
#include <windows.h>
#if !defined(ULONG_PTR)         //  usually defined in <BaseTsd.h>
#if !defined(_WIN64)
#define ULONG_PTR UINT
#else
#define ULONG_PTR UINT64
#endif // !defined(_WIN64)
#endif // !defined(ULONG_PTR)
#else
#include <stdint.h>
	typedef int32_t LONG;     //  LONG  is assumed to be 4 Bytes
	typedef uint32_t UINT;     //  UINT  is assumed to be 4 Bytes
	typedef uintptr_t ULONG_PTR;
#define __stdcall
#endif

long LCS2open(void);
void LCS2close(void);

/**
 * @brief Initialize sdk
 * @note Control instruction
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_INIT_DLL)(void);
extern LCS_INIT_DLL lcs_init_dll;
/**
 * @brief Release SDK
 * @note Control instruction
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_FREE_DLL)(void);
extern LCS_FREE_DLL lcs_free_dll;
/**
 * @brief Get last error code
 * @note Control instruction
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_GET_LAST_ERROR)(void);
extern LCS_GET_LAST_ERROR lcs_get_last_error;
/**
 * @brief Get last error code
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_GET_LAST_ERROR)(const uint32_t CardNo);
extern LCS_N_GET_LAST_ERROR lcs_n_get_last_error;
/**
 * @brief Get the number of managed USB boards
 * @note Control instruction
 * @return Number of boards
 */
typedef uint32_t (LCS_API *LCS_COUNT_CARDS)(void);
extern LCS_COUNT_CARDS lcs_count_cards;

/**
* @brief Find USB boards
* @note Control instruction
* @return Number of boards
*/
typedef uint32_t (LCS_API *LCS_SEARCH_CARDS)(void);
extern LCS_SEARCH_CARDS lcs_search_cards;
/**
* @brief Binding USB Card
* @note Control instruction
* @note Add the found board to the board manager
* @param SearchNo Found board serial number
* @param CardNo The Board ID bound to must be an ID that was not previously present in the Board Manager
* @return Error code
*/
typedef LCS2Error (LCS_API *LCS_ASSIGN_CARD)(const uint32_t SearchNo, const uint32_t CardNo);
extern LCS_ASSIGN_CARD lcs_assign_card;
/**
* @brief Unbind USB card
* @note Control instruction
* @note Remove the specified board from the Board Manager
* @param CardNo The bound board ID, must be an ID from the board manager
* @return Error code
*/
typedef LCS2Error (LCS_API *LCS_REMOVE_CARD)(const uint32_t CardNo);
extern LCS_REMOVE_CARD lcs_remove_card;
/**
* @brief Get the serial number of managed boards
* @note Control instruction
* @param szBuf buffer for serial number
* @param bufSize buffer size
* @return Error code
*/
typedef LCS2Error (LCS_API *LCS_GET_SERIAL_NUMBER)(char* szBuf, int32_t bufSize);
extern LCS_GET_SERIAL_NUMBER lcs_get_serial_number;

/**
* @brief Get the serial number of managed boards
* @note Control instruction
* @param CardNo Board ID
* @param szBuf buffer for serial number
* @param bufSize buffer size
* @return Error code
*/
typedef LCS2Error (LCS_API *LCS_N_GET_SERIAL_NUMBER)(const uint32_t CardNo, char* szBuf, int32_t bufSize);
extern LCS_N_GET_SERIAL_NUMBER lcs_n_get_serial_number;
/**
 * @brief Get card access
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_ACQUIRE_CARD)(const uint32_t CardNo);
extern LCS_ACQUIRE_CARD lcs_acquire_card;
/**
 * @brief Release card access
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_RELEASE_CARD)(const uint32_t CardNo);
extern LCS_RELEASE_CARD lcs_release_card;
/**
 * @brief Select default card
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SELECT_CARD)(const uint32_t CardNo);
extern LCS_SELECT_CARD lcs_select_card;
/**
 * @brief Start writing list
 * @note Control instruction
 * @param ListNo List ID, range: 1,2
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_START_LIST)(const uint32_t ListNo);
extern LCS_SET_START_LIST lcs_set_start_list;
/**
 * @brief Start writing list
 * @note Control instruction
 * @param CardNo Board ID
 * @param ListNo List ID, range: 1,2
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_START_LIST)(const uint32_t CardNo, const uint32_t ListNo);
extern LCS_N_SET_START_LIST lcs_n_set_start_list;
/**
 * @brief Load List
 * @note Control instruction
 * @param ListNo List ID, range: 1,2
 * @param Pos Loading location starts from 0
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_LOAD_LIST)(const uint32_t ListNo, const uint32_t Pos);
extern LCS_LOAD_LIST lcs_load_list;
/**
 * @brief Load List
 * @note Control instruction
 * @param CardNo Board ID
 * @param ListNo List ID, range: 1,2
 * @param Pos Loading location starts from 0
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_LOAD_LIST)(const uint32_t CardNo, const uint32_t ListNo, const uint32_t Pos);
extern LCS_N_LOAD_LIST lcs_n_load_list;
/**
 * @brief Load a sublist
 * @note Control instruction
 * @param Index Sublist number
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_LOAD_SUB)(const uint32_t Index);
extern LCS_LOAD_SUB lcs_load_sub;
/**
 * @brief Load a sublist
 * @note Control instruction
 * @param CardNo Board ID
 * @param Index Sublist number
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_LOAD_SUB)(const uint32_t CardNo, const uint32_t Index);
extern LCS_N_LOAD_SUB lcs_n_load_sub;
/**
 * @brief Call sublist
 * @note List Instruction
 * @param Index Sublist number
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SUB_CALL)(const uint32_t Index);
extern LCS_SUB_CALL lcs_sub_call;
/**
 * @brief Call sublist
 * @note List Instruction
 * @param CardNo Board ID
 * @param Index Sublist number
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SUB_CALL)(const uint32_t CardNo, const uint32_t Index);
extern LCS_N_SUB_CALL lcs_n_sub_call;
/**
 * @brief Load a character list
 * @note Control instruction
 * @param Char Character ascii code
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_LOAD_CHAR)(const uint32_t Char);
extern LCS_LOAD_CHAR lcs_load_char;
/**
 * @brief Load a character list
 * @note Control instruction
 * @param CardNo Board ID
 * @param Char Character ascii code
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_LOAD_CHAR)(const uint32_t CardNo, const uint32_t Char);
extern LCS_N_LOAD_CHAR lcs_n_load_char;
/**
 * @brief Mark a string of text
 * @note List Instruction
 * @param Text Text
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_MARK_TEXT)(const char* Text);
extern LCS_MARK_TEXT lcs_mark_text;
/**
 * @brief Mark a string of text
 * @note List Instruction
 * @param CardNo Board ID
 * @param Text Text
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_MARK_TEXT)(const uint32_t CardNo, const char* Text);
extern LCS_N_MARK_TEXT lcs_n_mark_text;

/**
* @brief Mark a string of text
* @note List Instruction
* @param Text Text
* @return Error code
*/
typedef LCS2Error(LCS_API *LCS_MARK_TEXT_ABS)(const char* Text);
extern LCS_MARK_TEXT_ABS lcs_mark_text_abs;
/**
* @brief Mark a string of text
* @note List Instruction
* @param CardNo Board ID
* @param Text Text
* @return Error code
*/
typedef LCS2Error(LCS_API *LCS_N_MARK_TEXT_ABS)(const uint32_t CardNo, const char* Text);
extern LCS_N_MARK_TEXT_ABS lcs_n_mark_text_abs;
/**
 * @brief Mark a character
 * @note List Instruction
 * @param Char Character ascii code
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_MARK_CHAR)(const uint32_t Char);
extern LCS_MARK_CHAR lcs_mark_char;
/**
 * @brief Mark a character
 * @note List Instruction
 * @param CardNo Board ID
 * @param Char Character ascii code
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_MARK_CHAR)(const uint32_t CardNo, const uint32_t Char);
extern LCS_N_MARK_CHAR lcs_n_mark_char;

/**
* @brief Mark a character
* @note List Instruction
* @param Char Character ascii code
* @return Error code
*/
typedef LCS2Error(LCS_API *LCS_MARK_CHAR_ABS)(const uint32_t Char);
extern LCS_MARK_CHAR_ABS lcs_mark_char_abs;
/**
* @brief Mark a character
* @note List Instruction
* @param CardNo Board ID
* @param Char Character ascii code
* @return Error code
*/
typedef LCS2Error(LCS_API *LCS_N_MARK_CHAR_ABS)(const uint32_t CardNo, const uint32_t Char);
extern LCS_N_MARK_CHAR_ABS lcs_n_mark_char_abs;
/**
 * @brief Get list write location
 * @note Control instruction
 * @param ListNo Current List ID, range: 1,2
 * @param Pos Current listWriting position starts from 0
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_GET_LIST_POINTER)(uint32_t* ListNo, uint32_t* Pos);
extern LCS_GET_LIST_POINTER lcs_get_list_pointer;

/**
 * @brief Get list write location
 * @note Control instruction
 * @param CardNo Board ID
 * @param ListNo Current List ID, range: 1,2
 * @param Pos Current listWriting position starts from 0
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_GET_LIST_POINTER)(const uint32_t CardNo, uint32_t* ListNo, uint32_t* Pos);
extern LCS_N_GET_LIST_POINTER lcs_n_get_list_pointer;
/**
 * @brief Get list write location
 * @note Control instruction
 * @return Current listWriting position starts from 0
 */
typedef uint32_t (LCS_API *LCS_GET_INPUT_POINTER)(void);
extern LCS_GET_INPUT_POINTER lcs_get_input_pointer;
/**
 * @brief Get list write location
 * @note Control instruction
 * @param CardNo Board ID
 * @return Current listWriting position starts from 0
 */
typedef uint32_t (LCS_API *LCS_N_GET_INPUT_POINTER)(const uint32_t CardNo);
extern LCS_N_GET_INPUT_POINTER lcs_n_get_input_pointer;
/**
 * @brief Execute list command at specified location
 * @note Control instruction
 * @param ListNo List ID, range: 1,2
 * @param Pos List execution position starts from 0
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_EXECUTE_LIST_POS)(const uint32_t ListNo, const uint32_t Pos);
extern LCS_EXECUTE_LIST_POS lcs_execute_list_pos;
/**
 * @brief Execute list command at specified location
 * @note Control instruction
 * @param CardNo Board ID
 * @param ListNo List ID, range: 1,2
 * @param Pos List execution position starts from 0
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_EXECUTE_LIST_POS)(const uint32_t CardNo, const uint32_t ListNo, const uint32_t Pos);
extern LCS_N_EXECUTE_LIST_POS lcs_n_execute_list_pos;
/**
 * @brief Execute from specified location
 * @note Control instruction
 * @param Pos List execution position starts from 0
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_EXECUTE_AT_POINTER)(const uint32_t Pos);
extern LCS_EXECUTE_AT_POINTER lcs_execute_at_pointer;
/**
 * @brief Execute from specified location
 * @note Control instruction
 * @param CardNo Board ID
 * @param Pos List execution position starts from 0
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_EXECUTE_AT_POINTER)(const uint32_t CardNo, const uint32_t Pos);
extern LCS_N_EXECUTE_AT_POINTER lcs_n_execute_at_pointer;
/**
 * @brief Execute list from position 0
 * @note Control instruction
 * @param ListNo List ID, range: 1,2
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_EXECUTE_LIST)(const uint32_t ListNo);
extern LCS_EXECUTE_LIST lcs_execute_list;
/**
 * @brief Execute list from position 0
 * @note Control instruction
 * @param CardNo Board ID
 * @param ListNo List ID, range: 1,2
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_EXECUTE_LIST)(const uint32_t CardNo, const uint32_t ListNo);
extern LCS_N_EXECUTE_LIST lcs_n_execute_list;
/**
 * @brief Specify the next list to execute
 * @note Only takes effect once by default
 * @note Control instruction
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_AUTO_CHANGE)();
extern LCS_AUTO_CHANGE lcs_auto_change;
/**
 * @brief Specify the next list to execute
 * @note Only takes effect once by default
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_AUTO_CHANGE)(const uint32_t CardNo);
extern LCS_N_AUTO_CHANGE lcs_n_auto_change;
/**
 * @brief Specify the position of the next list to be executed
 * @note Only takes effect once by default
 * @note Control instruction
 * @param Pos List execution position starts from 0
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_AUTO_CHANGE_POS)(const uint32_t Pos);
extern LCS_AUTO_CHANGE_POS lcs_auto_change_pos;
/**
 * @brief Specify the position of the next list to be executed
 * @note Only takes effect once by default
 * @note Control instruction
 * @param CardNo Board ID
 * @param Pos List execution position starts from 0
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_AUTO_CHANGE_POS)(const uint32_t CardNo, const uint32_t Pos);
extern LCS_N_AUTO_CHANGE_POS lcs_n_auto_change_pos;
/**
 * @brief Start list loop
 * @note Let auto_change always take effect
 * @note Control instruction
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_START_LOOP)(void);
extern LCS_START_LOOP lcs_start_loop;
/**
 * @brief Start list loop
 * @note Let auto_change always take effect
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_START_LOOP)(const uint32_t CardNo);
extern LCS_N_START_LOOP lcs_n_start_loop;
/**
 * @brief Terminate list loop
 * @note Let auto_ Change only takes effect once
 * @note Control instruction
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_QUIT_LOOP)(void);
extern LCS_QUIT_LOOP lcs_quit_loop;
/**
 * @brief Terminate list loop
 * @note Let auto_ Change only takes effect once
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_QUIT_LOOP)(const uint32_t CardNo);
extern LCS_N_QUIT_LOOP lcs_n_quit_loop;
/**
 * @brief Pause instruction list execution
 * @note Control instruction
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_PAUSE_LIST)(void);
extern LCS_PAUSE_LIST lcs_pause_list;

/**
 * @brief Pause instruction list execution
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_PAUSE_LIST)(const uint32_t CardNo);
extern LCS_N_PAUSE_LIST lcs_n_pause_list;
/**
 * @brief Restore instruction list execution
 * @note Control instruction
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_RESTART_LIST)(void);
extern LCS_RESTART_LIST lcs_restart_list;
/**
 * @brief Restore instruction list execution
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_RESTART_LIST)(const uint32_t CardNo);
extern LCS_N_RESTART_LIST lcs_n_restart_list;
/**
 * @brief Stop command execution
 * @note Control instruction
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_STOP_EXECUTION)(void);
extern LCS_STOP_EXECUTION lcs_stop_execution;
/**
 * @brief Stop command execution
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_STOP_EXECUTION)(const uint32_t CardNo);
extern LCS_N_STOP_EXECUTION lcs_n_stop_execution;
/**
 * @brief Get list status
 * @note Control instruction
 * @param[out] uStatus List status (value is the ListStatus structure)
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_READ_STATUS)(uint32_t* uStatus);
extern LCS_READ_STATUS lcs_read_status;
/**
 * @brief Get list status
 * @note Control instruction
 * @param CardNo Board ID
 * @param[out] uStatus List status (value is the ListStatus structure)
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_READ_STATUS)(const uint32_t CardNo, uint32_t* uStatus);
extern LCS_N_READ_STATUS lcs_n_read_status;
/**
 * @brief Get list execution status
 * @note Control instruction
 * @param[out] Status Execution status (value in the BoardRunStatus structure)
 * @param[out] Pos List execution position
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_GET_STATUS)(uint32_t* Status, uint32_t* Pos);
extern LCS_GET_STATUS lcs_get_status;
/**
 * @brief Get list execution status
 * @note Control instruction
 * @param CardNo Board ID
 * @param[out] Status Execution status (value in the BoardRunStatus structure)
 * @param[out] Pos List execution position
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_GET_STATUS)(const uint32_t CardNo, uint32_t* Status, uint32_t* Pos);
extern LCS_N_GET_STATUS lcs_n_get_status;
///////////////////////////////////////IO and DA related

/**
 * @brief Set output port level based on mask
 * @note Control instruction
 * @param Value	The IO port level from low to high represents ports 0~31 respectively.
 * @param Mask The IO port mask represents ports 0~31 from low to high.
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_WRITE_IO_PORT_MASK)(const uint32_t Value, const uint32_t Mask);
extern LCS_WRITE_IO_PORT_MASK lcs_write_io_port_mask;
/**
 * @brief Set output port level based on mask
 * @note Control instruction
 * @param CardNo Board ID
 * @param Value	The IO port level from low to high represents ports 0~31 respectively.
 * @param Mask The IO port mask represents ports 0~31 from low to high.
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_WRITE_IO_PORT_MASK)(const uint32_t CardNo, const uint32_t Value, const uint32_t Mask);         
extern LCS_N_WRITE_IO_PORT_MASK lcs_n_write_io_port_mask;
/**
 * @brief Read the IO input port level
 * @note Control instruction
 * @return The IO port level from low to high represents ports 0~31 respectively.
 */
typedef uint32_t (LCS_API *LCS_READ_IO_PORT)(void);
extern LCS_READ_IO_PORT lcs_read_io_port;
/**
 * @brief Read the IO input port level
 * @note Control instruction
 * @param CardNo Board ID
 * @return The IO port level from low to high represents ports 0~31 respectively.
 */
typedef uint32_t (LCS_API *LCS_N_READ_IO_PORT)(const uint32_t CardNo);
extern LCS_N_READ_IO_PORT lcs_n_read_io_port;
/**
 * @brief Read IO output port level
 * @note Control instruction
 * @return The IO port level from low to high represents ports 0~31 respectively.
 */
typedef uint32_t (LCS_API *LCS_GET_IO_STATUS)(void);
extern LCS_GET_IO_STATUS lcs_get_io_status;
/**
 * @brief Read IO output port level
 * @note Control instruction
 * @param CardNo Board ID
 * @return The IO port level from low to high represents ports 0~31 respectively.
 */
typedef uint32_t (LCS_API *LCS_N_GET_IO_STATUS)(const uint32_t CardNo);
extern LCS_N_GET_IO_STATUS lcs_n_get_io_status;
/**
 * @brief Set DA value
 * @note Control instruction
 * @param x DA port selection, range 1,2
 * @param Value DA percentage range 0~100
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_WRITE_DA_X)(const uint32_t x, const uint32_t Value);
extern LCS_WRITE_DA_X lcs_write_da_x;
/**
 * @brief Set DA value
 * @note Control instruction
 * @param CardNo Board ID
 * @param x DA port selection, range 1,2
 * @param Value  DA percentage range 0~100
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_WRITE_DA_X)(const uint32_t CardNo, const uint32_t x, const uint32_t Value);
extern LCS_N_WRITE_DA_X lcs_n_write_da_x;
/**
 * @brief Set output port level
 * @note Control instruction
 * @param Value The IO port level from low to high represents ports 0~31 respectively.
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_WRITE_IO_PORT)(const uint32_t Value);
extern LCS_WRITE_IO_PORT lcs_write_io_port;
/**
 * @brief Set output port level
 * @note Control instruction
 * @param CardNo Board ID
 * @param Value The IO port level from low to high represents ports 0~31 respectively.
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_WRITE_IO_PORT)(const uint32_t CardNo, const uint32_t Value);
extern LCS_N_WRITE_IO_PORT lcs_n_write_io_port;
//////////////////////////////////////Light control
/**
 * @brief Turn off laser MO
 * @note List Instruction
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_DISABLE_LASER)(void);
extern LCS_DISABLE_LASER lcs_disable_laser;
/**
 * @brief Turn off laser MO
 * @note List Instruction
 * @param CardNo Board ID
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_DISABLE_LASER)(const uint32_t CardNo);
extern LCS_N_DISABLE_LASER lcs_n_disable_laser;
/**
 * @brief Turn On Laser MO
 * @note List Instruction
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_ENABLE_LASER)(void);
extern LCS_ENABLE_LASER lcs_enable_laser;
/**
 * @brief Turn On Laser MO
 * @note List Instruction
 * @param CardNo Board ID
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_ENABLE_LASER)(const uint32_t CardNo);
extern LCS_N_ENABLE_LASER lcs_n_enable_laser;
/**
 * @brief Set pre ionization cycle and pulse width
 * @note Control instruction
 * @param Period Period unit:us range:1~1365us
 * @param PulseLength Pulse width unit:us range:1~1000us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_STANDBY)(const uint32_t Period, const uint32_t PulseLength);
extern LCS_SET_STANDBY lcs_set_standby;
/**
 * @brief Set pre ionization cycle and pulse width
 * @note Control instruction
 * @param CardNo Board ID
 * @param Period Period unit:us range:1~1365us
 * @param PulseLength Pulse width unit:us range:1~1000us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_STANDBY)(const uint32_t CardNo, const uint32_t Period, const uint32_t PulseLength);
extern LCS_N_SET_STANDBY lcs_n_set_standby;
/**
 * @brief Get pre-ionization period and pulse width
 * @note Control instruction
 * @param Period Period unit:us range:1~1365us
 * @param PulseLength Pulse width unit:us range:1~1000us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_GET_STANDBY)(uint32_t* Period, uint32_t* PulseLength);
extern LCS_GET_STANDBY lcs_get_standby;
/**
 * @brief Get pre-ionization period and pulse width
 * @note Control instruction
 * @param CardNo Board ID
 * @param Period Period unit:us range:1~1365us
 * @param PulseLength Pulse width unit:us range:1~1000us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_GET_STANDBY)(const uint32_t CardNo, uint32_t* Period, uint32_t* PulseLength);
extern LCS_N_GET_STANDBY lcs_n_get_standby;
/**
 * @brief Set the light cycle and pulse width
 * @note Control instruction
 * @param Period Period unit:us
 * @param PulseLength Pulse width unit:us
 * @param mopaPulse mopa Pulse width unit ns
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_LASER_PULSES_CTRL)(const uint32_t Period, const uint32_t PulseLength, const uint16_t mopaPulse);
extern LCS_SET_LASER_PULSES_CTRL lcs_set_laser_pulses_ctrl;
/**
 * @brief Set the light cycle and pulse width
 * @note Control instruction
 * @param CardNo Board ID
 * @param Period Period unit:us
 * @param PulseLength Pulse width unit:us
 * @param mopaPulse mopa Pulse width unit ns
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_LASER_PULSES_CTRL)(const uint32_t CardNo, const uint32_t Period, const uint32_t PulseLength, const uint16_t mopaPulse);
extern LCS_N_SET_LASER_PULSES_CTRL lcs_n_set_laser_pulses_ctrl;
/**
 * @brief Set the first pulse suppression time for YAG and UV
 * @note Control instruction
 * @param Length Suppression time unit:us range:0~15000us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_FIRSTPULSE_KILLER)(const uint32_t Length);
extern LCS_SET_FIRSTPULSE_KILLER lcs_set_firstpulse_killer;
/**
 * @brief Set the first pulse suppression time for YAG and UV
 * @note Control instruction
 * @param CardNo Board ID
 * @param Length  Suppression time unit:us range:0~15000us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_FIRSTPULSE_KILLER)(const uint32_t CardNo, const uint32_t Length);
extern LCS_N_SET_FIRSTPULSE_KILLER lcs_n_set_firstpulse_killer;
/**
 * @brief Set on hysteresis
 * @note Control instruction
 * @param Delay Lag time unit:us range:0~512us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_QSWITCH_DELAY)(const int32_t Delay);
extern LCS_SET_QSWITCH_DELAY lcs_set_qswitch_delay;
/**
 * @brief Set on hysteresis
 * @note Control instruction
 * @param CardNo Board ID
 * @param Delay Lag time unit:us range:0~512us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_QSWITCH_DELAY)(const uint32_t CardNo, const int32_t Delay);
extern LCS_N_SET_QSWITCH_DELAY lcs_n_set_qswitch_delay;
/**
 * @brief Set laser mode
 * @note Control instruction
 * @param Mode Laser type (LCS2LaserType enumeration)
 * @param bRedLight Whether red light mode
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_LASER_MODE)(const uint32_t Mode, bool bRedLight);
extern LCS_SET_LASER_MODE lcs_set_laser_mode;

/**
 * @brief Set laser mode
 * @note Control instruction
 * @param CardNo Board ID
 * @param Mode Laser type (LCS2LaserType enumeration)
 * @param bRedLight Whether red light mode
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_LASER_MODE)(const uint32_t CardNo, const uint32_t Mode, bool bRedLight);
extern LCS_N_SET_LASER_MODE lcs_n_set_laser_mode;
/**
 * @brief Enable laser output/turn off
 * @note Control instruction
 * @param bEnable Whether to enable
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_LASER_CONTROL)(bool bEnable);
extern LCS_SET_LASER_CONTROL lcs_set_laser_control;
/**
 * @brief Enable laser output/turn off
 * @note Control instruction
 * @param CardNo Board ID
 * @param bEnable Whether to enable
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_LASER_CONTROL)(const uint32_t CardNo, bool bEnable);
extern LCS_N_SET_LASER_CONTROL lcs_n_set_laser_control;
///////////////////////////////////////Galvanometer control related
/**
 * @brief Set the position of the galvanometer movement after marking
 * @note Control instruction
 * @param XHome Position x coordinate
 * @param YHome Position y coordinate
 * @return Error code
 */
typedef LCS2Error (LCS_API* LCS_HOME_POSITION)(const double XHome, const double YHome);
extern LCS_HOME_POSITION lcs_home_position;
/**
* @brief Set the position of the galvanometer movement after marking
* @note Control instruction
* @param CardNo Board ID
* @param XHome Position x coordinate
* @param YHome Position y coordinate
* @return Error code
*/
typedef LCS2Error (LCS_API *LCS_N_HOME_POSITION)(const uint32_t CardNo, const double XHome, const double YHome);
extern LCS_N_HOME_POSITION lcs_n_home_position;
/**
 * @brief Move the galvanometer position
 * @note Control instruction
 * @param X X coordinate, unit: mm
 * @param Y Y coordinate, unit: mm
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_GOTO_XY)(const double X, const double Y);
extern LCS_GOTO_XY lcs_goto_xy;
/**
 * @brief Move the galvanometer position
 * @note Control instruction
 * @param CardNo Board ID
 * @param X X coordinate, unit: mm
 * @param Y Y coordinate, unit: mm
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_GOTO_XY)(const uint32_t CardNo, const double X, const double Y);
extern LCS_N_GOTO_XY lcs_n_goto_xy;
/**
 * @brief Set marking offset
 * @note Control instruction
 * @param HeadNo The galvanometer number is temporarily unavailable
 * @param XOffset X offset in millimeters
 * @param YOffset Y offset in millimeters
 * @param at_once Only effective once, temporarily useless
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_OFFSET)(const uint32_t HeadNo, const double XOffset, const double YOffset, const uint32_t at_once);
extern LCS_SET_OFFSET lcs_set_offset;
/**
 * @brief Set marking offset
 * @note Control instruction
 * @param CardNo Board ID
 * @param HeadNo The galvanometer number is temporarily unavailable
 * @param XOffset X offset in millimeters
 * @param YOffset Y offset in millimeters
 * @param at_once Only effective once, temporarily useless
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_OFFSET)(const uint32_t CardNo, const uint32_t HeadNo, const double XOffset, const double YOffset, const uint32_t at_once);
extern LCS_N_SET_OFFSET lcs_n_set_offset;
/**
 * @brief Set marking rotation
 * @note Control instruction
 * @param HeadNo The galvanometer number is temporarily unavailable
 * @param Angle Angle counterclockwise is positive
 * @param at_once Only effective once, temporarily useless
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_ANGLE)(const uint32_t HeadNo, const double Angle, const uint32_t at_once);
extern LCS_SET_ANGLE lcs_set_angle;
/**
 * @brief n_set_angle Set marking rotation
 * @note Control instruction
 * @param CardNo Board ID
 * @param HeadNo The galvanometer number is temporarily unavailable
 * @param Angle Angle counterclockwise is positive
 * @param at_once Only effective once, temporarily useless
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_ANGLE)(const uint32_t CardNo, const uint32_t HeadNo, const double Angle, const uint32_t at_once);
extern LCS_N_SET_ANGLE lcs_n_set_angle;
///////////////////////////////////////Galvanometer control related
/**
 * @brief Set delay mode
 * @note Control instruction
 * @param VarPoly Whether to enableVariable corner delay
 * @param MinJumpDelay Minimum jump delay unit:us range:0~65535us
 * @param MaxJumpDelay  Maximum jump delay unit:us range:0~65535us
 * @param JumpLengthLimit Maximum jump distance in mm. When it is 0, the variable jump delay is turned off.
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_DELAY_MODE)(const bool VarPoly, const int32_t MinJumpDelay, const int32_t MaxJumpDelay, const double JumpLengthLimit);
extern LCS_SET_DELAY_MODE lcs_set_delay_mode;
/**
 * @brief Set delay mode
 * @note Control instruction
 * @param CardNo Board ID
 * @param VarPoly Whether to enableVariable corner delay
 * @param MinJumpDelay  Minimum jump delay unit:us range:0~65535us
 * @param MaxJumpDelay  Maximum jump delay unit:us range:0~65535us
 * @param JumpLengthLimit Maximum jump distance  unit mm  when it is 0, the variable jump delay is turned off
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_DELAY_MODE)(const uint32_t CardNo, const bool VarPoly, const int32_t MinJumpDelay, const int32_t MaxJumpDelay, const double JumpLengthLimit);
extern LCS_N_SET_DELAY_MODE lcs_n_set_delay_mode;
/**
 * @brief Set jump speed
 * @note Control instruction
 * @param Speed Speed unit mm/s
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_JUMP_SPEED_CTRL)(const double Speed);
extern LCS_SET_JUMP_SPEED_CTRL lcs_set_jump_speed_ctrl;
/**
 * @brief Set jump speed
 * @note Control instruction
 * @param CardNo Board ID
 * @param Speed Speed unit mm/s
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_JUMP_SPEED_CTRL)(const uint32_t CardNo, const double Speed);
extern LCS_N_SET_JUMP_SPEED_CTRL lcs_n_set_jump_speed_ctrl;
/**
 * @brief Set marking speed
 * @note Control instruction
 * @param Speed Speed unit mm/s
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_MARK_SPEED_CTRL)(const double Speed);
extern LCS_SET_MARK_SPEED_CTRL lcs_set_mark_speed_ctrl;
/**
 * @brief Set marking speed
 * @note Control instruction
 * @param CardNo Board ID
 * @param Speed Speed unit mm/s
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_MARK_SPEED_CTRL)(const uint32_t CardNo, const double Speed);
extern LCS_N_SET_MARK_SPEED_CTRL lcs_n_set_mark_speed_ctrl;
/////////////////////////////////////////List Instruction Related
/**
 * @brief Enable galvanometer correction parameter file
 * @note List Instruction
 * @param bEnableA Whether to enable the calibration file of galvanometer 1
 * @param bEnableB Whether to enable the calibration file of galvanometer 2 is temporarily unavailable.
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SELECT_COR_TABLE_LIST)(const bool bEnableA, const bool bEnableB);
extern LCS_SELECT_COR_TABLE_LIST lcs_select_cor_table_list;
/**
 * @brief Enable galvanometer correction parameter file
 * @note List Instruction
 * @param CardNo Board ID
 * @param bEnableA Whether to enable the calibration file of galvanometer 1
 * @param bEnableB Whether to enable the calibration file of galvanometer 2 is temporarily unavailable.
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SELECT_COR_TABLE_LIST)(const uint32_t CardNo, const bool bEnableA, const bool bEnableB);
extern LCS_N_SELECT_COR_TABLE_LIST lcs_n_select_cor_table_list;

/**
 * @brief Delay for a while
 * @note List Instruction
 * @param Delay Delay time unit us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_LONG_DELAY)(const uint32_t Delay);
extern LCS_LONG_DELAY lcs_long_delay;
/**
 * @brief Delay for a while
 * @note List Instruction
 * @param CardNo Board ID
 * @param Delay Delay time unit us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_LONG_DELAY)(const uint32_t CardNo, const uint32_t Delay);
extern LCS_N_LONG_DELAY lcs_n_long_delay;
/**
 * @brief Set the end of the current list
 * @note List Instruction
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_END_OF_LIST)(void);
extern LCS_SET_END_OF_LIST lcs_set_end_of_list;
/**
 * @brief Set the end of the current list
 * @note List Instruction
 * @param CardNo Board ID
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_END_OF_LIST)(const uint32_t CardNo);
extern LCS_N_SET_END_OF_LIST lcs_n_set_end_of_list;
/**
 * @brief Execute from specified location
 * @note List Instruction
 * @param Pos Jump to absolute position starting from 0
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_LIST_JUMP_POS)(const uint32_t Pos);
extern LCS_LIST_JUMP_POS lcs_list_jump_pos;
/**
 * @brief Execute from specified location
 * @note List Instruction
 * @param CardNo Board ID
 * @param Pos Jump to absolute position starting from 0
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_LIST_JUMP_POS)(const uint32_t CardNo, const uint32_t Pos);
extern LCS_N_LIST_JUMP_POS lcs_n_list_jump_pos;
/**
 * @brief The list Jump to relative position
 * @note List Instruction
 * @param Pos Jumping to a relative position of 0 indicates no jumping
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_LIST_JUMP_REL)(const long Pos);
extern LCS_LIST_JUMP_REL lcs_list_jump_rel;
/**
 * @brief The list Jump to relative position
 * @note List Instruction
 * @param CardNo Board ID
 * @param Pos Jumping to a relative position of 0 indicates no jumping
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_LIST_JUMP_REL)(const uint32_t CardNo, const long Pos);
extern LCS_N_LIST_JUMP_REL lcs_n_list_jump_rel;
/**
 * @brief The List ready for repeat execution
 * @note List Instruction
 * @note Execute to list_ Until
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_LIST_REPEAT)(void);
extern LCS_LIST_REPEAT lcs_list_repeat;
/**
 * @brief The List ready for repeat execution
 * @note List Instruction
 * @note Execute to list_ Until
 * @param CardNo Board ID
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_LIST_REPEAT)(const uint32_t CardNo);
extern LCS_N_LIST_REPEAT lcs_n_list_repeat;
/**
 * @brief The List starts repeating execution
 * @note List Instruction
 * @note Repeat the instructions starting from list_repeat
 * @param Number The minimum number of repetitions is 1
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_LIST_UNTIL)(const uint32_t Number);
extern LCS_LIST_UNTIL lcs_list_until;
/**
 * @brief The List starts repeating execution
 * @note List Instruction
 * @note Repeat the instructions starting from list_repeat
 * @param CardNo Board ID
 * @param Number The minimum number of repetitions is 1
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_LIST_UNTIL)(const uint32_t CardNo, const uint32_t Number);
extern LCS_N_LIST_UNTIL lcs_n_list_until;

/**
 * @brief Sublist return
 * @note Control instruction
 * @note Valid for load_char and load_sub
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_LIST_RETURN)(void);
extern LCS_LIST_RETURN lcs_list_return;
/**
 * @brief Sublist return
 * @note Control instruction
 * @note Valid for load_char and load_sub
 * @param CardNo Board ID
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_LIST_RETURN)(const uint32_t CardNo);
extern LCS_N_LIST_RETURN lcs_n_list_return;
///////////////////////////////////////IO and DA related
/**
 * @brief Set output port level based on mask
 * @note List Instruction
 * @param Value The port level from low to high represents ports 0~31 respectively.
 * @param Mask The port mask represents ports 0~31 from low to high.
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_WRITE_IO_PORT_MASK_LIST)(const uint32_t Value, const uint32_t Mask);
extern LCS_WRITE_IO_PORT_MASK_LIST lcs_write_io_port_mask_list;
/**
 * @brief Set output port level based on mask
 * @note List Instruction
 * @param CardNo Board ID
 * @param Value The port level from low to high represents ports 0~31 respectively.
 * @param Mask The port mask represents ports 0~31 from low to high.
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_WRITE_IO_PORT_MASK_LIST)(const uint32_t CardNo, const uint32_t Value, const uint32_t Mask);
extern LCS_N_WRITE_IO_PORT_MASK_LIST lcs_n_write_io_port_mask_list;
/**
 * @brief Set all output port levels
 * @note List Instruction
 * @param Value The port level from low to high represents ports 0~31 respectively.
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_WRITE_IO_PORT_LIST)(const uint32_t Value);
extern LCS_WRITE_IO_PORT_LIST lcs_write_io_port_list;
/**
 * @brief Set all output port levels
 * @note List Instruction
 * @param CardNo Board ID
 * @param Value The port level from low to high represents ports 0~31 respectively.
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_WRITE_IO_PORT_LIST)(const uint32_t CardNo, const uint32_t Value);
extern LCS_N_WRITE_IO_PORT_LIST lcs_n_write_io_port_list;
/**
 * @brief Set the value of DA
 * @note List Instruction
 * @param x DA port selection, range 1,2
 * @param Value DA percentage range 0~100
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_WRITE_DA_X_LIST)(const uint32_t x, const uint32_t Value);
extern LCS_WRITE_DA_X_LIST lcs_write_da_x_list;
/**
 * @brief Set the value of DA
 * @note List Instruction
 * @param CardNo Board ID
 * @param x DA port selection, range 1,2
 * @param Value DA percentage range 0~100
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_WRITE_DA_X_LIST)(const uint32_t CardNo, const uint32_t x, const uint32_t Value);
extern LCS_N_WRITE_DA_X_LIST lcs_n_write_da_x_list;
//////////////////////////////////////Light control

/**
 * @brief Mark a point
 * @note List Instruction
 * @param Period RBI time unit us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_LASER_ON_LIST)(const uint32_t Period);
extern LCS_LASER_ON_LIST lcs_laser_on_list;
/**
 * @brief Mark a point
 * @note List Instruction
 * @param CardNo Board ID
 * @param Period RBI time unit us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_LASER_ON_LIST)(const uint32_t CardNo, const uint32_t Period);
extern LCS_N_LASER_ON_LIST lcs_n_laser_on_list;
/**
 * @brief Set on/off light delay
 * @note List Instruction
 * @param LaserOnDelay Opening delay unit: us range:-32768~32767us
 * @param LaserOffDelay Off light delay unit:us range:0~65535us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_LASER_DELAYS)(const int32_t LaserOnDelay, const int32_t LaserOffDelay);
extern LCS_SET_LASER_DELAYS lcs_set_laser_delays;
/**
 * @brief Set on/off light delay
 * @note List Instruction
 * @param CardNo Board ID
 * @param LaserOnDelay Opening delay unit: us range:-32768~32767us
 * @param LaserOffDelay Off light delay unit:us range:0~65535us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_LASER_DELAYS)(const uint32_t CardNo, const int32_t LaserOnDelay, const int32_t LaserOffDelay);
extern LCS_N_SET_LASER_DELAYS lcs_n_set_laser_delays;
/**
 * @brief Set pre ionization cycle and pulse width
 * @note List Instruction
 * @param Period Period unit:us range:1~1365us
 * @param PulseLength Pulse width unit:us range:1~1000us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_STANDBY_LIST)(const uint32_t Period, const uint32_t PulseLength);
extern LCS_SET_STANDBY_LIST lcs_set_standby_list;
/**
 * @brief Set pre ionization cycle and pulse width
 * @note List Instruction
 * @param CardNo Board ID
 * @param Period Period unit:us range:1~1365us
 * @param PulseLength Pulse width unit:us range:1~1000us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_STANDBY_LIST)(const uint32_t CardNo, const uint32_t Period, const uint32_t PulseLength);
extern LCS_N_SET_STANDBY_LIST lcs_n_set_standby_list;
/**
* @brief Set pwm cycle and pulse width
* @note List Instruction
* @param Period Period unit us
* @param PulseLength Pulse width unit us
* @param mopaPulse mopa Pulse width unit ns
* @return Error code
*/
typedef LCS2Error (LCS_API *LCS_SET_LASER_PULSES)(const uint32_t Period, const uint32_t PulseLength, const uint16_t mopaPulse);
extern LCS_SET_LASER_PULSES lcs_set_laser_pulses;
/**
* @brief Set pwm cycle and pulse width
* @note List Instruction
* @param CardNo Board ID
* @param Period Period unit us
* @param PulseLength Pulse width unit us
* @param mopaPulse mopa Pulse width unit ns
* @return Error code
*/
typedef LCS2Error (LCS_API *LCS_N_SET_LASER_PULSES)(const uint32_t CardNo, const uint32_t Period, const uint32_t PulseLength, const uint16_t mopaPulse);
extern LCS_N_SET_LASER_PULSES lcs_n_set_laser_pulses;
/**
 * @brief Set the first pulse suppression time for YAG and UV
 * @note List Instruction
 * @param Length First pulse suppression time unit:us range:0~15000us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_FIRSTPULSE_KILLER_LIST)(const uint32_t Length);
extern LCS_SET_FIRSTPULSE_KILLER_LIST lcs_set_firstpulse_killer_list;
/**
 * @brief Set the first pulse suppression time for YAG and UV
 * @note List Instruction
 * @param CardNo Board ID
 * @param Length First pulse suppression time unit:us range:0~15000us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_FIRSTPULSE_KILLER_LIST)(const uint32_t CardNo, const uint32_t Length);
extern LCS_N_SET_FIRSTPULSE_KILLER_LIST lcs_n_set_firstpulse_killer_list;
/**
 * @brief Set on hysteresis
 * @note List Instruction
 * @param Delay  Lag time unit:us range:0~512us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_QSWITCH_DELAY_LIST)(const int32_t Delay);
extern LCS_SET_QSWITCH_DELAY_LIST lcs_set_qswitch_delay_list;
/**
 * @brief Set on hysteresis
 * @note List Instruction
 * @param CardNo Board ID
 * @param Delay Lag time unit:us range:0~512us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_QSWITCH_DELAY_LIST)(const uint32_t CardNo, const int32_t Delay);
extern LCS_N_SET_QSWITCH_DELAY_LIST lcs_n_set_qswitch_delay_list;

/**
 * @brief set wobble pattern
 * @note List Instruction
 * @param Transversal Jitter y-direction length (perpendicular to the marking direction) unit: mm
 * @param Longitudinal Shake the length in the x-direction (parallel to the marking direction) unit: mm
 * @param Space Jitter spacing unit: mm
 * @param Mode Dither pattern
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_WOBBLE_MODE)(const double Transversal, const double Longitudinal, const double Space, const WobbleType Mode);
extern LCS_SET_WOBBLE_MODE lcs_set_wobble_mode;
/**
 * @brief set wobble pattern
 * @note List Instruction
 * @param CardNo Board ID
 * @param Transversal Jitter y-direction length (perpendicular to the marking direction) unit: mm
 * @param Longitudinal Shake the length in the x-direction (parallel to the marking direction) unit: mm
 * @param Space Jitter spacing unit: mm
 * @param Mode Dither pattern
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_WOBBLE_MODE)(const uint32_t CardNo, const double Transversal, const double Longitudinal, const double Space, const WobbleType Mode);
extern LCS_N_SET_WOBBLE_MODE lcs_n_set_wobble_mode;
///////////////////////////////////////Galvanometer control related
/**
 * @brief Mark to absolute position
 * @note List Instruction
 * @param X Coordinate x, unit mm
 * @param Y Coordinate y, unit mm
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_MARK_ABS)(const double X, const double Y);
extern LCS_MARK_ABS lcs_mark_abs;
/**
 * @brief Mark to absolute position
 * @note List Instruction
 * @param CardNo Board ID
 * @param X Coordinate x, unit mm
 * @param Y Coordinate y, unit mm
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_MARK_ABS)(const uint32_t CardNo, const double X, const double Y);
extern LCS_N_MARK_ABS lcs_n_mark_abs;
/**
 * @brief Mark to relative position
 * @note List Instruction
 * @param dX Coordinate offset x, in mm
 * @param dY Coordinate offset y, in mm
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_MARK_REL)(const double dX, const double dY);
extern LCS_MARK_REL lcs_mark_rel;
/**
 * @brief Mark to relative position
 * @note List Instruction
 * @param CardNo Board ID
 * @param dX Coordinate offset x, in mm
 * @param dY Coordinate offset y, in mm
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_MARK_REL)(const uint32_t CardNo, const double dX, const double dY);
extern LCS_N_MARK_REL lcs_n_mark_rel;
///////////////////////////////////////Galvanometer control related
/**
 * @brief Jump to absolute position
 * @note List Instruction
 * @param X Coordinate x, unit mm
 * @param Y Coordinate y, unit mm
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_JUMP_ABS)(const double X, const double Y);
extern LCS_JUMP_ABS lcs_jump_abs;
/**
 * @brief Jump to absolute position
 * @note List Instruction
 * @param CardNo Board ID
 * @param X Coordinate x, unit mm
 * @param Y Coordinate y, unit mm
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_JUMP_ABS)(const uint32_t CardNo, const double X, const double Y);
extern LCS_N_JUMP_ABS lcs_n_jump_abs;
/**
 * @brief Jump to relative position
 * @note List Instruction
 * @param dX
 * @param dY
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_JUMP_REL)(const double dX, const double dY);
extern LCS_JUMP_REL lcs_jump_rel;
/**
 * @brief Jump to relative position
 * @note List Instruction
 * @param CardNo Board ID
 * @param dX Coordinate offset x, in mm
 * @param dY Coordinate offset y, in mm
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_JUMP_REL)(const uint32_t CardNo, const double dX, const double dY);
extern LCS_N_JUMP_REL lcs_n_jump_rel;
///////////////////////////////////////Galvanometer offset rotation List Instruction

/**
 * @brief Set marking offset
 * @note List Instruction
 * @param HeadNo The galvanometer number is temporarily unavailable
 * @param XOffset Offset in x direction, unit mm
 * @param YOffset Offset in y direction, unit mm
 * @param at_once Is it only valid once, temporarily useless
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_OFFSET_LIST)(const uint32_t HeadNo, const double XOffset, const double YOffset, const uint32_t at_once);
extern LCS_SET_OFFSET_LIST lcs_set_offset_list;
/**
 * @brief Set marking offset
 * @note List Instruction
 * @param CardNo Board ID
 * @param HeadNo The galvanometer number is temporarily unavailable
 * @param XOffset Offset in x direction, unit mm
 * @param YOffset Offset in y direction, unit mm
 * @param at_once Is it only valid once, temporarily useless
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_OFFSET_LIST)(const uint32_t CardNo, const uint32_t HeadNo, const double XOffset, const double YOffset, const uint32_t at_once);
extern LCS_N_SET_OFFSET_LIST lcs_n_set_offset_list;
/**
 * @brief Set marking angle
 * @note List Instruction
 * @param HeadNo The galvanometer number is temporarily unavailable
 * @param Angle Rotation angle, counterclockwise is positive
 * @param at_once Is it only valid once, temporarily useless
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_ANGLE_LIST)(const uint32_t HeadNo, const double Angle, const uint32_t at_once);
extern LCS_SET_ANGLE_LIST lcs_set_angle_list;
/**
 * @brief Set marking angle
 * @note List Instruction
 * @param CardNo Board ID
 * @param HeadNo The galvanometer number is temporarily unavailable
 * @param Angle Rotation angle, counterclockwise is positive
 * @param at_once Is it only valid once, temporarily useless
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_ANGLE_LIST)(const uint32_t CardNo, const uint32_t HeadNo, const double Angle, const uint32_t at_once);
extern LCS_N_SET_ANGLE_LIST lcs_n_set_angle_list;
///////////////////////////////////////Galvanometer control (speed) related
/**
 * @brief Set marking speed
 * @note List Instruction
 * @param Speed Speed, unit mm/s
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_MARK_SPEED)(const double Speed);
extern LCS_SET_MARK_SPEED lcs_set_mark_speed;
/**
 * @brief Set marking speed
 * @note List Instruction
 * @param CardNo Board ID
 * @param Speed Speed, unit mm/s
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_MARK_SPEED)(const uint32_t CardNo, const double Speed);
extern LCS_N_SET_MARK_SPEED lcs_n_set_mark_speed;
/**
 * @brief Set jump speed
 * @note List Instruction
 * @param Speed Speed, unit mm/s
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_JUMP_SPEED)(const double Speed);
extern LCS_SET_JUMP_SPEED lcs_set_jump_speed;
/**
 * @brief Set jump speed
 * @note List Instruction
 * @param CardNo Board ID
 * @param Speed Speed, unit mm/s
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_JUMP_SPEED)(const uint32_t CardNo, const double Speed);
extern LCS_N_SET_JUMP_SPEED lcs_n_set_jump_speed;
/**
 * @brief Set galvanometer delay
 * @note List Instruction
 * @param Mark Marking delay, unit:us range:0~65535us
 * @param Polygon Corner delay, unit:us range:0~65535us
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_SCANNER_DELAYS)(const int32_t Mark, const int32_t Polygon);
extern LCS_SET_SCANNER_DELAYS lcs_set_scanner_delays;
/**
 * @brief Set galvanometer delay
 * @note List Instruction
 * @param CardNo Board ID
 * @param Mark Marking delay, unit:us range:0~65535us
 * @param Polygon Corner delay, unit:us range:0~65535us
 * @return  Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_SCANNER_DELAYS)(const uint32_t CardNo, const int32_t Mark, const int32_t Polygon);
extern LCS_N_SET_SCANNER_DELAYS lcs_n_set_scanner_delays;
/**
 * @brief Load calibration file
 * @note Control instruction
 * @param FileName Correct the full path of the file
 * @param bEnable Whether to enable calibration file
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_LOAD_CORRECTION_FILE)(const char* FileName, const bool bEnable);
extern LCS_LOAD_CORRECTION_FILE lcs_load_correction_file;
/**
 * @brief Load calibration file
 * @note Control instruction
 * @param CardNo Board ID
 * @param FileName Correct the full path of the file
 * @param bEnable Whether to enable correction files
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_LOAD_CORRECTION_FILE)(const uint32_t CardNo, const char* FileName, const bool bEnable);
extern LCS_N_LOAD_CORRECTION_FILE lcs_n_load_correction_file;
/**
 * @brief Enable galvanometer correction parameter file
 * @note Control instruction
 * @param bEnableA Whether to enable coordinate correction of galvanometer 1
 * @param bEnableB Whether to enable coordinate correction of galvanometer 2, temporarily useless
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SELECT_COR_TABLE)(const bool bEnableA, const bool bEnableB);
extern LCS_SELECT_COR_TABLE lcs_select_cor_table;
/**
 * @brief Enable galvanometer correction parameter file
 * @note Control instruction
 * @param CardNo Board ID
 * @param bEnableA Whether to enable coordinate correction of galvanometer 1
 * @param bEnableB Whether to enable coordinate correction of galvanometer 2, temporarily useless
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SELECT_COR_TABLE)(const uint32_t CardNo, const bool bEnableA, const bool bEnableB);
extern LCS_N_SELECT_COR_TABLE lcs_n_select_cor_table;
/**
 * @brief Set manual calibration parameters
 * @note Control instruction
 * @param scaleX Zoom in x direction
 * @param scaleY scaling in y direction
 * @param bucketX x-direction barrel distortion
 * @param bucketY y-direction barrel distortion
 * @param paralleX x-direction parallelogram distortion
 * @param paralleY y-direction parallelogram distortion
 * @param trapeX x-direction trapezoidal distortion
 * @param trapeY y-direction trapezoidal distortion
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_MANUAL_CORRECTION_PARAMS)(double scaleX, double scaleY, double bucketX, double bucketY, double paralleX, double paralleY, double trapeX, double trapeY);
extern LCS_SET_MANUAL_CORRECTION_PARAMS lcs_set_manual_correction_params;
/**
 * @brief Set manual calibration parameters
 * @note Control instruction
 * @param CardNo Board ID
 * @param scaleX Zoom in x direction
 * @param scaleY scaling in y direction
 * @param bucketX x-direction barrel distortion
 * @param bucketY y-direction barrel distortion
 * @param paralleX x-direction parallelogram distortion
 * @param paralleY y-direction parallelogram distortion
 * @param trapeX x-direction trapezoidal distortion
 * @param trapeY y-direction trapezoidal distortion
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_MANUAL_CORRECTION_PARAMS)(const uint32_t CardNo, double scaleX, double scaleY, double bucketX, double bucketY, double paralleX, double paralleY, double trapeX, double trapeY);
extern LCS_N_SET_MANUAL_CORRECTION_PARAMS lcs_n_set_manual_correction_params;
/**
 * @brief Set galvanometer parameters
 * @note Control instruction
 * @param fWorkSize Galvanometer area size in mm
 * @param bXyFlip xy interchange
 * @param bXInvert x reverse
 * @param bYInvert y reverse
 * @param angle Rotation angle unit: angle Counterclockwise is positive
 * @param x_offset X-direction offset
 * @param y_offset Y-direction offset
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_SCANAHEAD_PARAMS)(double fWorkSize, bool bXyFlip, bool bXInvert, bool bYInvert, double angle, double x_offset, double y_offset);
extern LCS_SET_SCANAHEAD_PARAMS lcs_set_scanahead_params;
/**
 * @brief Set galvanometer parameters
 * @note Control instruction
 * @param CardNo Board ID
 * @param fWorkSize Galvanometer area size in mm
 * @param bXyFlip xy interchange
 * @param bXInvert x reverse
 * @param bYInvert y reverse
 * @param angle Rotation angle unit: angle Counterclockwise is positive
 * @param x_offset X-direction offset
 * @param y_offset Y-direction offset
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_SCANAHEAD_PARAMS)(const uint32_t CardNo, double fWorkSize, bool bXyFlip, bool bXInvert, bool bYInvert, double angle, double x_offset, double y_offset);
extern LCS_N_SET_SCANAHEAD_PARAMS lcs_n_set_scanahead_params;
/**
 * @brief Clear marking count
 * @note Control instruction
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_CLEAR_MARK_COUNT)(void);
extern LCS_CLEAR_MARK_COUNT lcs_clear_mark_count;
/**
 * @brief Clear marking count
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_CLEAR_MARK_COUNT)(const uint32_t CardNo);
extern LCS_N_CLEAR_MARK_COUNT lcs_n_clear_mark_count;
/**
 * @brief Obtain marking count
 * @note Control instruction
 * @return Marking count
 */
typedef uint32_t (LCS_API *LCS_GET_MARK_COUNT)(void);
extern LCS_GET_MARK_COUNT lcs_get_mark_count;
/**
 * @brief Obtain marking count
 * @note Control instruction
 * @param CardNo Board ID
 * @return Marking count
 */
typedef uint32_t (LCS_API *LCS_N_GET_MARK_COUNT)(const uint32_t CardNo);
extern LCS_N_GET_MARK_COUNT lcs_n_get_mark_count;
/**
 * @brief Marking times plus one
 * @note List Instruction
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_INC_MARK_COUNT)(void);
extern LCS_INC_MARK_COUNT lcs_inc_mark_count;
/**
 * @brief Marking times plus one
 * @note List Instruction
 * @param CardNo Board ID
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_INC_MARK_COUNT)(const uint32_t CardNo);
extern LCS_N_INC_MARK_COUNT lcs_n_inc_mark_count;
/**
 * @brief Set fiber/mopa power
 * @note List Instruction
 * @param power Power percentage, range 0~100
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_LASER_POWER)(unsigned char power);
extern LCS_SET_LASER_POWER lcs_set_laser_power;
/**
 * @brief Set fiber/mopa power
 * @note List Instruction
 * @param CardNo Board ID
 * @param power Power percentage, range 0~100
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_LASER_POWER)(const uint32_t CardNo, unsigned char power);
extern LCS_N_SET_LASER_POWER lcs_n_set_laser_power;

/**
 * @brief The expansion axis starts moving
 * @note List Instruction
 * @param nAxisId Axis ID, range 0,1
 * @param fPulse Number of motion pulses
 * @param bOppDir Is reverse true=reverse false=forward
 * @param RunSpeed Movement speed unit:pulse/second
 * @param startSpeed Starting speed unit: pulse/second
 * @param accTime Acceleration time unit: ms Range: 0~255
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_AXIS_MOVE)(int nAxisId, double fPulse, bool bOppDir, double RunSpeed, double startSpeed, uint16_t accTime);
extern LCS_SET_AXIS_MOVE lcs_set_axis_move;
/**
 * @brief The expansion axis starts moving
 * @note List Instruction
 * @param CardNo Board ID
 * @param nAxisId Axis ID, range 0,1
 * @param fPulse Number of motion pulses
 * @param bOppDir Is reverse true=reverse false=forward
 * @param RunSpeed Movement speed unit:pulse/second
 * @param startSpeed Starting speed unit: pulse/second
 * @param accTime Acceleration time unit: ms Range: 0~255
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_AXIS_MOVE)(const uint32_t CardNo, int nAxisId, double fPulse, bool bOppDir, double RunSpeed, double startSpeed, uint16_t accTime);
extern LCS_N_SET_AXIS_MOVE lcs_n_set_axis_move;
/**
 * @brief The extended axis starts returning to zero
 * @note List Instruction
 * @param nAxisId Axis ID, range 0,1
 * @param fPulse Number of motion pulses
 * @param bOppDir Whether to return to zero in the reverse direction true=reverse false=forward
 * @param RunSpeed Zero return speed unit: pulse/second
 * @param startSpeed Starting speed unit: pulse/second
 * @param accTime Acceleration time unit: ms Range: 0~255
 * @param nZeroType Zero level true=high false=low
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_SET_AXIS_TOZERO)(int nAxisId, double fPulse, bool bOppDir, double RunSpeed, double startSpeed, uint16_t accTime, bool nZeroType);
extern LCS_SET_AXIS_TOZERO lcs_set_axis_tozero;
/**
 * @brief The extended axis starts returning to zero
 * @note List Instruction
 * @param CardNo Board ID
 * @param nAxisId Axis ID, range 0,1
 * @param fPulse Number of motion pulses
 * @param bOppDir Whether to return to zero in the reverse direction true=reverse false=forward
 * @param RunSpeed Zero return speed unit: pulse/second
 * @param startSpeed Starting speed unit: pulse/second
 * @param accTime Acceleration time unit: ms Range: 0~255
 * @param nZeroType Zero level true=high false=low
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_SET_AXIS_TOZERO)(const uint32_t CardNo, int nAxisId, double fPulse, bool bOppDir,double RunSpeed, double startSpeed, uint16_t accTime, bool nZeroType);
extern LCS_N_SET_AXIS_TOZERO lcs_n_set_axis_tozero;
/**
* @brief Get Extension Axis Status
* @note Control instruction
* @param Status Return axis status, AxisStatus structure
* @param PosX Current position of the x-axis extension, unit:pulse defaults to 0x40000000
* @param PosY Current position of the y-axis extension, unit:pulse defaults to 0x40000000
* @return Error code
*/
typedef LCS2Error (LCS_API* LCS_GET_AXIS_STATUS)(uint32_t* Status, uint32_t* PosX, uint32_t* PosY);
extern LCS_GET_AXIS_STATUS lcs_get_axis_status;

/**
* @brief Get Extension Axis Status
* @note Control instruction
* @param CardNo Board ID
* @param Status Return axis status, AxisStatus structure
* @param PosX Current position of the x-axis extension, unit:pulse defaults to 0x40000000
* @param PosY Current position of the y-axis extension, unit:pulse defaults to 0x40000000
* @return Error code
*/
typedef LCS2Error (LCS_API* LCS_N_GET_AXIS_STATUS)(const uint32_t CardNo, uint32_t* Status, uint32_t* PosX, uint32_t* PosY);
extern LCS_N_GET_AXIS_STATUS lcs_n_get_axis_status;
/////////////////////////////////////////////The following is the API related to the network port card//////////////////////////////////////////////////////////////////////
/**
 * @brief Set card search timeout
 * @note Control instruction
 * @param TimeOut Timeout unit ms
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_ETH_SET_SEARCH_CARDS_TIMEOUT)(const uint32_t TimeOut);
extern LCS_ETH_SET_SEARCH_CARDS_TIMEOUT lcs_eth_set_search_cards_timeout;
/**
 * @brief Count the number of network port cards
 * @note Control instruction
 * @return Number of boards
 */
typedef uint32_t (LCS_API *LCS_ETH_COUNT_CARDS)(void);
extern LCS_ETH_COUNT_CARDS lcs_eth_count_cards;
/**
 * @brief Send a broadcast to find the network port card
 * @note Control instruction
 * @param Ip Local network port IP, network byte order
 * @param NetMask Find the subnet mask used for the network interface card (such as 255.255.255.255 or 192.168.0.255), network byte order
 * @return Number of boards found
 */
typedef uint32_t (LCS_API *LCS_ETH_SEARCH_CARDS)(const uint32_t Ip, const uint32_t NetMask);
extern LCS_ETH_SEARCH_CARDS lcs_eth_search_cards;
/**
 * @brief Search for network interface cards within the specified IP segment
 * @note Control instruction
 * @param StartIp Starting IP, network byte order
 * @param EndIp End IP, network byte order
 * @return Number of boards found
 */
typedef uint32_t (LCS_API *LCS_ETH_SEARCH_CARDS_RANGE)(const uint32_t StartIp, const uint32_t EndIp);
extern LCS_ETH_SEARCH_CARDS_RANGE lcs_eth_search_cards_range;
/**
 * @brief Get the network port card IP
 * @note Control instruction
 * @param CardNo Board ID
 * @return IP,Network byte order
 */
typedef uint32_t (LCS_API *LCS_ETH_GET_IP)(const uint32_t CardNo);
extern LCS_ETH_GET_IP lcs_eth_get_ip;
/**
 * @brief Get the IP of the found card
 * @note Control instruction
 * @param SearchNo Found board serial number
 * @return IP,Network byte order
 */
typedef uint32_t (LCS_API *LCS_ETH_GET_IP_SEARCH)(const uint32_t SearchNo);
extern LCS_ETH_GET_IP_SEARCH lcs_eth_get_ip_search;
/**
 * @brief Set static IP
 * @note Control instruction
 * @param Ip IP address, network byte order
 * @param NetMask Subnet mask, network byte order
 * @param Gateway Gateway address, network byte order
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_ETH_SET_STATIC_IP)(const uint32_t Ip, const uint32_t NetMask, const uint32_t Gateway);
extern LCS_ETH_SET_STATIC_IP lcs_eth_set_static_ip;
/**
 * @brief Set static IP
 * @note Control instruction
 * @param CardNo Board ID
 * @param Ip IP address, network byte order
 * @param NetMask Subnet mask, network byte order
 * @param Gateway Gateway address, network byte order
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_ETH_SET_STATIC_IP)(const uint32_t CardNo, const uint32_t Ip, const uint32_t NetMask, const uint32_t Gateway);
extern LCS_N_ETH_SET_STATIC_IP lcs_n_eth_set_static_ip;
/**
 * @brief Get static IP
 * @note Control instruction
 * @param Ip IP address, network byte order
 * @param NetMask Subnet mask, network byte order
 * @param Gateway Gateway address, network byte order
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_ETH_GET_STATIC_IP)(uint32_t* Ip, uint32_t* NetMask, uint32_t* Gateway);
extern LCS_ETH_GET_STATIC_IP lcs_eth_get_static_ip;
/**
 * @brief Get static IP
 * @note Control instruction
 * @param CardNo Board ID
 * @param Ip IP address, network byte order
 * @param NetMask Subnet mask, network byte order
 * @param Gateway Gateway address, network byte order
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_N_ETH_GET_STATIC_IP)(const uint32_t CardNo, uint32_t* Ip, uint32_t* NetMask, uint32_t* Gateway);
extern LCS_N_ETH_GET_STATIC_IP lcs_n_eth_get_static_ip;
/**
 * @brief Binding Card
 * @note Control instruction
 * @note Add the found board to the board manager
 * @param Ip IP address, network byte order
 * @param CardNo The Board ID bound to must be an ID that was not previously present in the Board Manager
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_ETH_ASSIGN_CARD_IP)(const uint32_t Ip, const uint32_t CardNo);
extern LCS_ETH_ASSIGN_CARD_IP lcs_eth_assign_card_ip;
/**
 * @brief Binding UDP Card
 * @note Control instruction
 * @note Add the found board to the board manager
 * @param SearchNo Found board serial number
 * @param CardNo The Board ID bound to must be an ID that was not previously present in the Board Manager
 * @return Error code
 */
typedef LCS2Error (LCS_API *LCS_ETH_ASSIGN_CARD)(const uint32_t SearchNo, const uint32_t CardNo);
extern LCS_ETH_ASSIGN_CARD lcs_eth_assign_card;

/**
* @brief Unbind UDP card
* @note Control instruction
* @note Remove the specified board from the Board Manager
* @param CardNo The bound board ID, must be an ID from the board manager
* @return Error code
*/
typedef LCS2Error (LCS_API* LCS_ETH_REMOVE_CARD)(const uint32_t CardNo);
extern LCS_ETH_REMOVE_CARD lcs_eth_remove_card;

#if defined(__cplusplus)
}			//extern "C" 
#endif

bool lcs_connect();
bool lcs_available();