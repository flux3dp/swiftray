// lcsdll Implicitly linked header files
#include "public.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief Initialize sdk
 * @note Control instruction
 * @return Error code
 */
LCS_IMPORT LCS2Error  LCS_API init_dll(void);

/**
 * @brief Release SDK
 * @note Control instruction
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API free_dll(void);

/**
 * @brief Get last error code
 * @note Control instruction
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API get_last_error(void);

/**
 * @brief Get last error code
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_get_last_error(const uint32_t CardNo);

/**
 * @brief Get the number of managed USB boards
 * @note Control instruction
 * @return Number of boards
 */
LCS_IMPORT uint32_t LCS_API count_cards(void);

/**
* @brief Find USB boards
* @note Control instruction
* @return Number of boards
*/
LCS_IMPORT uint32_t LCS_API search_cards(void);

/**
* @brief Binding USB Card
* @note Control instruction
* @note Add the found board to the board manager
* @param SearchNo Found board serial number
* @param CardNo The Board ID bound to must be an ID that was not previously present in the Board Manager
* @return Error code
*/
LCS_IMPORT LCS2Error LCS_API assign_card(const uint32_t SearchNo, const uint32_t CardNo);

/**
* @brief Unbind USB card
* @note Control instruction
* @note Remove the specified board from the Board Manager
* @param CardNo The bound board ID, must be an ID from the board manager
* @return Error code
*/
LCS_IMPORT LCS2Error LCS_API remove_card(const uint32_t CardNo);

/**
* @brief Get the serial number of managed boards
* @note Control instruction
* @param szBuf buffer for serial number
* @param bufSize buffer size
* @return Error code
*/
LCS_IMPORT LCS2Error LCS_API get_serial_number(char* szBuf, int32_t bufSize);

/**
* @brief Get the serial number of managed boards
* @note Control instruction
* @param CardNo Board ID
* @param szBuf buffer for serial number
* @param bufSize buffer size
* @return Error code
*/
LCS_IMPORT LCS2Error LCS_API n_get_serial_number(const uint32_t CardNo, char* szBuf, int32_t bufSize);

/**
 * @brief Get card access
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API acquire_card(const uint32_t CardNo);

/**
 * @brief Release card access
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API release_card(const uint32_t CardNo);

/**
 * @brief Select default card
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API select_card(const uint32_t CardNo);

/**
 * @brief Start writing list
 * @note Control instruction
 * @param ListNo List ID, range: 1,2
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_start_list(const uint32_t ListNo);

/**
 * @brief Start writing list
 * @note Control instruction
 * @param CardNo Board ID
 * @param ListNo List ID, range: 1,2
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_start_list(const uint32_t CardNo, const uint32_t ListNo);

/**
 * @brief Load List
 * @note Control instruction
 * @param ListNo List ID, range: 1,2
 * @param Pos Loading location starts from 0
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API load_list(const uint32_t ListNo, const uint32_t Pos);

/**
 * @brief Load List
 * @note Control instruction
 * @param CardNo Board ID
 * @param ListNo List ID, range: 1,2
 * @param Pos Loading location starts from 0
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_load_list(const uint32_t CardNo, const uint32_t ListNo, const uint32_t Pos);

/**
 * @brief Load a sublist
 * @note Control instruction
 * @param Index Sublist number
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API load_sub(const uint32_t Index);

/**
 * @brief Load a sublist
 * @note Control instruction
 * @param CardNo Board ID
 * @param Index Sublist number
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_load_sub(const uint32_t CardNo, const uint32_t Index);

/**
 * @brief Call sublist
 * @note List Instruction
 * @param Index Sublist number
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API sub_call(const uint32_t Index);

/**
 * @brief Call sublist
 * @note List Instruction
 * @param CardNo Board ID
 * @param Index Sublist number
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_sub_call(const uint32_t CardNo, const uint32_t Index);

/**
 * @brief Load a character list
 * @note Control instruction
 * @param Char Character ascii code
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API load_char(const uint32_t Char);

/**
 * @brief Load a character list
 * @note Control instruction
 * @param CardNo Board ID
 * @param Char Character ascii code
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_load_char(const uint32_t CardNo, const uint32_t Char);

/**
 * @brief Mark a string of text
 * @note List Instruction
 * @param Text Text
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API mark_text(const char* Text);

/**
 * @brief Mark a string of text
 * @note List Instruction
 * @param CardNo Board ID
 * @param Text Text
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_mark_text(const uint32_t CardNo, const char* Text);

/**
* @brief Mark a string of text
* @note List Instruction
* @param Text Text
* @return Error code
*/
LCS_IMPORT LCS2Error LCS_API mark_text_abs(const char* Text);

/**
* @brief Mark a string of text
* @note List Instruction
* @param CardNo Board ID
* @param Text Text
* @return Error code
*/
LCS_IMPORT LCS2Error LCS_API n_mark_text_abs(const uint32_t CardNo, const char* Text);

/**
 * @brief Mark a character
 * @note List Instruction
 * @param Char Character ascii code
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API mark_char(const uint32_t Char);

/**
 * @brief Mark a character
 * @note List Instruction
 * @param CardNo Board ID
 * @param Char Character ascii code
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_mark_char(const uint32_t CardNo, const uint32_t Char);

/**
* @brief Mark a character
* @note List Instruction
* @param Char Character ascii code
* @return Error code
*/
LCS_IMPORT LCS2Error LCS_API mark_char_abs(const uint32_t Char);

/**
* @brief Mark a character
* @note List Instruction
* @param CardNo Board ID
* @param Char Character ascii code
* @return Error code
*/
LCS_IMPORT LCS2Error LCS_API n_mark_char_abs(const uint32_t CardNo, const uint32_t Char);

/**
 * @brief Get list write location
 * @note Control instruction
 * @param ListNo Current List ID, range: 1,2
 * @param Pos Current listWriting position starts from 0
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API get_list_pointer(uint32_t* ListNo, uint32_t* Pos);

/**
 * @brief Get list write location
 * @note Control instruction
 * @param CardNo Board ID
 * @param ListNo Current List ID, range: 1,2
 * @param Pos Current listWriting position starts from 0
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_get_list_pointer(const uint32_t CardNo, uint32_t* ListNo, uint32_t* Pos);

/**
 * @brief Get list write location
 * @note Control instruction
 * @return Current listWriting position starts from 0
 */
LCS_IMPORT uint32_t LCS_API get_input_pointer(void);

/**
 * @brief Get list write location
 * @note Control instruction
 * @param CardNo Board ID
 * @return Current listWriting position starts from 0
 */
LCS_IMPORT uint32_t LCS_API n_get_input_pointer(const uint32_t CardNo);

/**
 * @brief Execute list command at specified location
 * @note Control instruction
 * @param ListNo List ID, range: 1,2
 * @param Pos List execution position starts from 0
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API execute_list_pos(const uint32_t ListNo, const uint32_t Pos);

/**
 * @brief Execute list command at specified location
 * @note Control instruction
 * @param CardNo Board ID
 * @param ListNo List ID, range: 1,2
 * @param Pos List execution position starts from 0
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_execute_list_pos(const uint32_t CardNo, const uint32_t ListNo, const uint32_t Pos);

/**
 * @brief Execute from specified location
 * @note Control instruction
 * @param Pos List execution position starts from 0
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API execute_at_pointer(const uint32_t Pos);

/**
 * @brief Execute from specified location
 * @note Control instruction
 * @param CardNo Board ID
 * @param Pos List execution position starts from 0
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_execute_at_pointer(const uint32_t CardNo, const uint32_t Pos);

/**
 * @brief Execute list from position 0
 * @note Control instruction
 * @param ListNo List ID, range: 1,2
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API execute_list(const uint32_t ListNo);

/**
 * @brief Execute list from position 0
 * @note Control instruction
 * @param CardNo Board ID
 * @param ListNo List ID, range: 1,2
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_execute_list(const uint32_t CardNo, const uint32_t ListNo);

/**
 * @brief Specify the next list to execute
 * @note Only takes effect once by default
 * @note Control instruction
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API auto_change();

/**
 * @brief Specify the next list to execute
 * @note Only takes effect once by default
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_auto_change(const uint32_t CardNo);

/**
 * @brief Specify the position of the next list to be executed
 * @note Only takes effect once by default
 * @note Control instruction
 * @param Pos List execution position starts from 0
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API auto_change_pos(const uint32_t Pos);

/**
 * @brief Specify the position of the next list to be executed
 * @note Only takes effect once by default
 * @note Control instruction
 * @param CardNo Board ID
 * @param Pos List execution position starts from 0
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_auto_change_pos(const uint32_t CardNo, const uint32_t Pos);

/**
 * @brief Start list loop
 * @note Let auto_change always take effect
 * @note Control instruction
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API start_loop(void);

/**
 * @brief Start list loop
 * @note Let auto_change always take effect
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_start_loop(const uint32_t CardNo);

/**
 * @brief Terminate list loop
 * @note Let auto_ Change only takes effect once
 * @note Control instruction
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API quit_loop(void);

/**
 * @brief Terminate list loop
 * @note Let auto_ Change only takes effect once
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_quit_loop(const uint32_t CardNo);

/**
 * @brief Pause instruction list execution
 * @note Control instruction
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API pause_list(void);

/**
 * @brief Pause instruction list execution
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_pause_list(const uint32_t CardNo);

/**
 * @brief Restore instruction list execution
 * @note Control instruction
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API restart_list(void);

/**
 * @brief Restore instruction list execution
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_restart_list(const uint32_t CardNo);

/**
 * @brief Stop command execution
 * @note Control instruction
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API stop_execution(void);

/**
 * @brief Stop command execution
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_stop_execution(const uint32_t CardNo);

/**
 * @brief Get list status
 * @note Control instruction
 * @param[out] uStatus List status (value is the ListStatus structure)
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API read_status(uint32_t* uStatus);

/**
 * @brief Get list status
 * @note Control instruction
 * @param CardNo Board ID
 * @param[out] uStatus List status (value is the ListStatus structure)
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_read_status(const uint32_t CardNo, uint32_t* uStatus);

/**
 * @brief Get list execution status
 * @note Control instruction
 * @param[out] Status Execution status (value in the BoardRunStatus structure)
 * @param[out] Pos List execution position
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API get_status(uint32_t* Status, uint32_t* Pos);

/**
 * @brief Get list execution status
 * @note Control instruction
 * @param CardNo Board ID
 * @param[out] Status Execution status (value in the BoardRunStatus structure)
 * @param[out] Pos List execution position
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_get_status(const uint32_t CardNo, uint32_t* Status, uint32_t* Pos);

///////////////////////////////////////IO and DA related

/**
 * @brief Set output port level based on mask
 * @note Control instruction
 * @param Value	The IO port level from low to high represents ports 0~31 respectively.
 * @param Mask The IO port mask represents ports 0~31 from low to high.
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API write_io_port_mask(const uint32_t Value, const uint32_t Mask);

/**
 * @brief Set output port level based on mask
 * @note Control instruction
 * @param CardNo Board ID
 * @param Value	The IO port level from low to high represents ports 0~31 respectively.
 * @param Mask The IO port mask represents ports 0~31 from low to high.
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_write_io_port_mask(const uint32_t CardNo, const uint32_t Value, const uint32_t Mask);         

/**
 * @brief Read the IO input port level
 * @note Control instruction
 * @return The IO port level from low to high represents ports 0~31 respectively.
 */
LCS_IMPORT uint32_t LCS_API read_io_port(void);

/**
 * @brief Read the IO input port level
 * @note Control instruction
 * @param CardNo Board ID
 * @return The IO port level from low to high represents ports 0~31 respectively.
 */
LCS_IMPORT uint32_t LCS_API n_read_io_port(const uint32_t CardNo);

/**
 * @brief Read IO output port level
 * @note Control instruction
 * @return The IO port level from low to high represents ports 0~31 respectively.
 */
LCS_IMPORT uint32_t LCS_API get_io_status(void);

/**
 * @brief Read IO output port level
 * @note Control instruction
 * @param CardNo Board ID
 * @return The IO port level from low to high represents ports 0~31 respectively.
 */
LCS_IMPORT uint32_t LCS_API n_get_io_status(const uint32_t CardNo);

/**
 * @brief Set DA value
 * @note Control instruction
 * @param x DA port selection, range 1,2
 * @param Value DA percentage range 0~100
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API write_da_x(const uint32_t x, const uint32_t Value);

/**
 * @brief Set DA value
 * @note Control instruction
 * @param CardNo Board ID
 * @param x DA port selection, range 1,2
 * @param Value  DA percentage range 0~100
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_write_da_x(const uint32_t CardNo, const uint32_t x, const uint32_t Value);

/**
 * @brief Set output port level
 * @note Control instruction
 * @param Value The IO port level from low to high represents ports 0~31 respectively.
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API write_io_port(const uint32_t Value);

/**
 * @brief Set output port level
 * @note Control instruction
 * @param CardNo Board ID
 * @param Value The IO port level from low to high represents ports 0~31 respectively.
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_write_io_port(const uint32_t CardNo, const uint32_t Value);

//////////////////////////////////////Light control
/**
 * @brief Turn off laser MO
 * @note List Instruction
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API disable_laser(void);

/**
 * @brief Turn off laser MO
 * @note List Instruction
 * @param CardNo Board ID
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_disable_laser(const uint32_t CardNo);

/**
 * @brief Turn On Laser MO
 * @note List Instruction
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API enable_laser(void);

/**
 * @brief Turn On Laser MO
 * @note List Instruction
 * @param CardNo Board ID
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_enable_laser(const uint32_t CardNo);

/**
 * @brief Set pre ionization cycle and pulse width
 * @note Control instruction
 * @param Period Period unit:us range:1~1365us
 * @param PulseLength Pulse width unit:us range:1~1000us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_standby(const uint32_t Period, const uint32_t PulseLength);

/**
 * @brief Set pre ionization cycle and pulse width
 * @note Control instruction
 * @param CardNo Board ID
 * @param Period Period unit:us range:1~1365us
 * @param PulseLength Pulse width unit:us range:1~1000us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_standby(const uint32_t CardNo, const uint32_t Period, const uint32_t PulseLength);

/**
 * @brief Get pre-ionization period and pulse width
 * @note Control instruction
 * @param Period Period unit:us range:1~1365us
 * @param PulseLength Pulse width unit:us range:1~1000us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API get_standby(uint32_t* Period, uint32_t* PulseLength);

/**
 * @brief Get pre-ionization period and pulse width
 * @note Control instruction
 * @param CardNo Board ID
 * @param Period Period unit:us range:1~1365us
 * @param PulseLength Pulse width unit:us range:1~1000us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_get_standby(const uint32_t CardNo, uint32_t* Period, uint32_t* PulseLength);

/**
 * @brief Set the light cycle and pulse width
 * @note Control instruction
 * @param Period Period unit:us
 * @param PulseLength Pulse width unit:us
 * @param mopaPulse mopa Pulse width unit ns
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_laser_pulses_ctrl(const uint32_t Period, const uint32_t PulseLength, const uint16_t mopaPulse);

/**
 * @brief Set the light cycle and pulse width
 * @note Control instruction
 * @param CardNo Board ID
 * @param Period Period unit:us
 * @param PulseLength Pulse width unit:us
 * @param mopaPulse mopa Pulse width unit ns
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_laser_pulses_ctrl(const uint32_t CardNo, const uint32_t Period, const uint32_t PulseLength, const uint16_t mopaPulse);

/**
 * @brief Set the first pulse suppression time for YAG and UV
 * @note Control instruction
 * @param Length Suppression time unit:us range:0~15000us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_firstpulse_killer(const uint32_t Length);

/**
 * @brief Set the first pulse suppression time for YAG and UV
 * @note Control instruction
 * @param CardNo Board ID
 * @param Length  Suppression time unit:us range:0~15000us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_firstpulse_killer(const uint32_t CardNo, const uint32_t Length);

/**
 * @brief Set on hysteresis
 * @note Control instruction
 * @param Delay Lag time unit:us range:0~512us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_qswitch_delay(const int32_t Delay);

/**
 * @brief Set on hysteresis
 * @note Control instruction
 * @param CardNo Board ID
 * @param Delay Lag time unit:us range:0~512us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_qswitch_delay(const uint32_t CardNo, const int32_t Delay);

/**
 * @brief Set laser mode
 * @note Control instruction
 * @param Mode Laser type (LCS2LaserType enumeration)
 * @param bRedLight Whether red light mode
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_laser_mode(const uint32_t Mode, bool bRedLight);

/**
 * @brief Set laser mode
 * @note Control instruction
 * @param CardNo Board ID
 * @param Mode Laser type (LCS2LaserType enumeration)
 * @param bRedLight Whether red light mode
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_laser_mode(const uint32_t CardNo, const uint32_t Mode, bool bRedLight);

/**
 * @brief Enable laser output/turn off
 * @note Control instruction
 * @param bEnable Whether to enable
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_laser_control(bool bEnable);

/**
 * @brief Enable laser output/turn off
 * @note Control instruction
 * @param CardNo Board ID
 * @param bEnable Whether to enable
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_laser_control(const uint32_t CardNo, bool bEnable);

///////////////////////////////////////Galvanometer control related
/**
 * @brief Set the position of the galvanometer movement after marking
 * @note Control instruction
 * @param XHome Position x coordinate
 * @param YHome Position y coordinate
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API home_position(const double XHome, const double YHome);

/**
* @brief Set the position of the galvanometer movement after marking
* @note Control instruction
* @param CardNo Board ID
* @param XHome Position x coordinate
* @param YHome Position y coordinate
* @return Error code
*/
LCS_IMPORT LCS2Error LCS_API n_home_position(const uint32_t CardNo, const double XHome, const double YHome);

/**
 * @brief Move the galvanometer position
 * @note Control instruction
 * @param X X coordinate, unit: mm
 * @param Y Y coordinate, unit: mm
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API goto_xy(const double X, const double Y);

/**
 * @brief Move the galvanometer position
 * @note Control instruction
 * @param CardNo Board ID
 * @param X X coordinate, unit: mm
 * @param Y Y coordinate, unit: mm
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_goto_xy(const uint32_t CardNo, const double X, const double Y);
///////////////////////////////////////Galvanometer offset rotation Control instruction
/**
 * @brief Set marking offset
 * @note Control instruction
 * @param HeadNo The galvanometer number is temporarily unavailable
 * @param XOffset X offset in millimeters
 * @param YOffset Y offset in millimeters
 * @param at_once Only effective once, temporarily useless
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_offset(const uint32_t HeadNo, const double XOffset, const double YOffset, const uint32_t at_once);

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
LCS_IMPORT LCS2Error LCS_API n_set_offset(const uint32_t CardNo, const uint32_t HeadNo, const double XOffset, const double YOffset, const uint32_t at_once);

/**
 * @brief Set marking rotation
 * @note Control instruction
 * @param HeadNo The galvanometer number is temporarily unavailable
 * @param Angle Angle counterclockwise is positive
 * @param at_once Only effective once, temporarily useless
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_angle(const uint32_t HeadNo, const double Angle, const uint32_t at_once);

/**
 * @brief n_set_angle Set marking rotation
 * @note Control instruction
 * @param CardNo Board ID
 * @param HeadNo The galvanometer number is temporarily unavailable
 * @param Angle Angle counterclockwise is positive
 * @param at_once Only effective once, temporarily useless
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_angle(const uint32_t CardNo, const uint32_t HeadNo, const double Angle, const uint32_t at_once);
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
LCS_IMPORT LCS2Error LCS_API set_delay_mode(const bool VarPoly, const int32_t MinJumpDelay, const int32_t MaxJumpDelay, const double JumpLengthLimit);

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
LCS_IMPORT LCS2Error LCS_API n_set_delay_mode(const uint32_t CardNo, const bool VarPoly, const int32_t MinJumpDelay, const int32_t MaxJumpDelay, const double JumpLengthLimit);

/**
 * @brief Set jump speed
 * @note Control instruction
 * @param Speed Speed unit mm/s
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_jump_speed_ctrl(const double Speed);

/**
 * @brief Set jump speed
 * @note Control instruction
 * @param CardNo Board ID
 * @param Speed Speed unit mm/s
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_jump_speed_ctrl(const uint32_t CardNo, const double Speed);

/**
 * @brief Set marking speed
 * @note Control instruction
 * @param Speed Speed unit mm/s
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_mark_speed_ctrl(const double Speed);

/**
 * @brief Set marking speed
 * @note Control instruction
 * @param CardNo Board ID
 * @param Speed Speed unit mm/s
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_mark_speed_ctrl(const uint32_t CardNo, const double Speed);

/////////////////////////////////////////List Instruction Related
/**
 * @brief Enable galvanometer correction parameter file
 * @note List Instruction
 * @param bEnableA Whether to enable the calibration file of galvanometer 1
 * @param bEnableB Whether to enable the calibration file of galvanometer 2 is temporarily unavailable.
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API select_cor_table_list(const bool bEnableA, const bool bEnableB);

/**
 * @brief Enable galvanometer correction parameter file
 * @note List Instruction
 * @param CardNo Board ID
 * @param bEnableA Whether to enable the calibration file of galvanometer 1
 * @param bEnableB Whether to enable the calibration file of galvanometer 2 is temporarily unavailable.
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_select_cor_table_list(const uint32_t CardNo, const bool bEnableA, const bool bEnableB);

/**
 * @brief Delay for a while
 * @note List Instruction
 * @param Delay Delay time unit us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API long_delay(const uint32_t Delay);

/**
 * @brief Delay for a while
 * @note List Instruction
 * @param CardNo Board ID
 * @param Delay Delay time unit us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_long_delay(const uint32_t CardNo, const uint32_t Delay);

/**
 * @brief Set the end of the current list
 * @note List Instruction
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_end_of_list(void);

/**
 * @brief Set the end of the current list
 * @note List Instruction
 * @param CardNo Board ID
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_end_of_list(const uint32_t CardNo);

/**
 * @brief Execute from specified location
 * @note List Instruction
 * @param Pos Jump to absolute position starting from 0
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API list_jump_pos(const uint32_t Pos);

/**
 * @brief Execute from specified location
 * @note List Instruction
 * @param CardNo Board ID
 * @param Pos Jump to absolute position starting from 0
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_list_jump_pos(const uint32_t CardNo, const uint32_t Pos);

/**
 * @brief The list Jump to relative position
 * @note List Instruction
 * @param Pos Jumping to a relative position of 0 indicates no jumping
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API list_jump_rel(const long Pos);

/**
 * @brief The list Jump to relative position
 * @note List Instruction
 * @param CardNo Board ID
 * @param Pos Jumping to a relative position of 0 indicates no jumping
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_list_jump_rel(const uint32_t CardNo, const long Pos);

/**
 * @brief The List ready for repeat execution
 * @note List Instruction
 * @note Execute to list_ Until
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API list_repeat(void);

/**
 * @brief The List ready for repeat execution
 * @note List Instruction
 * @note Execute to list_ Until
 * @param CardNo Board ID
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_list_repeat(const uint32_t CardNo);

/**
 * @brief The List starts repeating execution
 * @note List Instruction
 * @note Repeat the instructions starting from list_repeat
 * @param Number The minimum number of repetitions is 1
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API list_until(const uint32_t Number);

/**
 * @brief The List starts repeating execution
 * @note List Instruction
 * @note Repeat the instructions starting from list_repeat
 * @param CardNo Board ID
 * @param Number The minimum number of repetitions is 1
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_list_until(const uint32_t CardNo, const uint32_t Number);

/**
 * @brief Sublist return
 * @note Control instruction
 * @note Valid for load_char and load_sub
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API list_return(void);

/**
 * @brief Sublist return
 * @note Control instruction
 * @note Valid for load_char and load_sub
 * @param CardNo Board ID
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_list_return(const uint32_t CardNo);

///////////////////////////////////////IO and DA related
/**
 * @brief Set output port level based on mask
 * @note List Instruction
 * @param Value The port level from low to high represents ports 0~31 respectively.
 * @param Mask The port mask represents ports 0~31 from low to high.
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API write_io_port_mask_list(const uint32_t Value, const uint32_t Mask);

/**
 * @brief Set output port level based on mask
 * @note List Instruction
 * @param CardNo Board ID
 * @param Value The port level from low to high represents ports 0~31 respectively.
 * @param Mask The port mask represents ports 0~31 from low to high.
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_write_io_port_mask_list(const uint32_t CardNo, const uint32_t Value, const uint32_t Mask);

/**
 * @brief Set all output port levels
 * @note List Instruction
 * @param Value The port level from low to high represents ports 0~31 respectively.
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API write_io_port_list(const uint32_t Value);

/**
 * @brief Set all output port levels
 * @note List Instruction
 * @param CardNo Board ID
 * @param Value The port level from low to high represents ports 0~31 respectively.
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_write_io_port_list(const uint32_t CardNo, const uint32_t Value);

/**
 * @brief Set the value of DA
 * @note List Instruction
 * @param x DA port selection, range 1,2
 * @param Value DA percentage range 0~100
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API write_da_x_list(const uint32_t x, const uint32_t Value);

/**
 * @brief Set the value of DA
 * @note List Instruction
 * @param CardNo Board ID
 * @param x DA port selection, range 1,2
 * @param Value DA percentage range 0~100
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_write_da_x_list(const uint32_t CardNo, const uint32_t x, const uint32_t Value);
//////////////////////////////////////Light control

/**
 * @brief Mark a point
 * @note List Instruction
 * @param Period RBI time unit us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API laser_on_list(const uint32_t Period);

/**
 * @brief Mark a point
 * @note List Instruction
 * @param CardNo Board ID
 * @param Period RBI time unit us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_laser_on_list(const uint32_t CardNo, const uint32_t Period);

/**
 * @brief Set on/off light delay
 * @note List Instruction
 * @param LaserOnDelay Opening delay unit: us range:-32768~32767us
 * @param LaserOffDelay Off light delay unit:us range:0~65535us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_laser_delays(const int32_t LaserOnDelay, const int32_t LaserOffDelay);

/**
 * @brief Set on/off light delay
 * @note List Instruction
 * @param CardNo Board ID
 * @param LaserOnDelay Opening delay unit: us range:-32768~32767us
 * @param LaserOffDelay Off light delay unit:us range:0~65535us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_laser_delays(const uint32_t CardNo, const int32_t LaserOnDelay, const int32_t LaserOffDelay);

/**
 * @brief Set pre ionization cycle and pulse width
 * @note List Instruction
 * @param Period Period unit:us range:1~1365us
 * @param PulseLength Pulse width unit:us range:1~1000us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_standby_list(const uint32_t Period, const uint32_t PulseLength);

/**
 * @brief Set pre ionization cycle and pulse width
 * @note List Instruction
 * @param CardNo Board ID
 * @param Period Period unit:us range:1~1365us
 * @param PulseLength Pulse width unit:us range:1~1000us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_standby_list(const uint32_t CardNo, const uint32_t Period, const uint32_t PulseLength);

/**
* @brief Set pwm cycle and pulse width
* @note List Instruction
* @param Period Period unit us
* @param PulseLength Pulse width unit us
* @param mopaPulse mopa Pulse width unit ns
* @return Error code
*/
LCS_IMPORT LCS2Error LCS_API set_laser_pulses(const uint32_t Period, const uint32_t PulseLength, const uint16_t mopaPulse);

/**
* @brief Set pwm cycle and pulse width
* @note List Instruction
* @param CardNo Board ID
* @param Period Period unit us
* @param PulseLength Pulse width unit us
* @param mopaPulse mopa Pulse width unit ns
* @return Error code
*/
LCS_IMPORT LCS2Error LCS_API n_set_laser_pulses(const uint32_t CardNo, const uint32_t Period, const uint32_t PulseLength, const uint16_t mopaPulse);

/**
 * @brief Set the first pulse suppression time for YAG and UV
 * @note List Instruction
 * @param Length First pulse suppression time unit:us range:0~15000us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_firstpulse_killer_list(const uint32_t Length);

/**
 * @brief Set the first pulse suppression time for YAG and UV
 * @note List Instruction
 * @param CardNo Board ID
 * @param Length First pulse suppression time unit:us range:0~15000us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_firstpulse_killer_list(const uint32_t CardNo, const uint32_t Length);

/**
 * @brief Set on hysteresis
 * @note List Instruction
 * @param Delay  Lag time unit:us range:0~512us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_qswitch_delay_list(const int32_t Delay);

/**
 * @brief Set on hysteresis
 * @note List Instruction
 * @param CardNo Board ID
 * @param Delay Lag time unit:us range:0~512us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_qswitch_delay_list(const uint32_t CardNo, const int32_t Delay);

/**
 * @brief set wobble pattern
 * @note List Instruction
 * @param Transversal Jitter y-direction length (perpendicular to the marking direction) unit: mm
 * @param Longitudinal Shake the length in the x-direction (parallel to the marking direction) unit: mm
 * @param Space Jitter spacing unit: mm
 * @param Mode Dither pattern
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_wobble_mode(const double Transversal, const double Longitudinal, const double Space, const WobbleType Mode);

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
LCS_IMPORT LCS2Error LCS_API n_set_wobble_mode(const uint32_t CardNo, const double Transversal, const double Longitudinal, const double Space, const WobbleType Mode);
///////////////////////////////////////Galvanometer control related
/**
 * @brief Mark to absolute position
 * @note List Instruction
 * @param X Coordinate x, unit mm
 * @param Y Coordinate y, unit mm
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API mark_abs(const double X, const double Y);

/**
 * @brief Mark to absolute position
 * @note List Instruction
 * @param CardNo Board ID
 * @param X Coordinate x, unit mm
 * @param Y Coordinate y, unit mm
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_mark_abs(const uint32_t CardNo, const double X, const double Y);

/**
 * @brief Mark to relative position
 * @note List Instruction
 * @param dX Coordinate offset x, in mm
 * @param dY Coordinate offset y, in mm
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API mark_rel(const double dX, const double dY);

/**
 * @brief Mark to relative position
 * @note List Instruction
 * @param CardNo Board ID
 * @param dX Coordinate offset x, in mm
 * @param dY Coordinate offset y, in mm
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_mark_rel(const uint32_t CardNo, const double dX, const double dY);

///////////////////////////////////////Galvanometer control related
/**
 * @brief Jump to absolute position
 * @note List Instruction
 * @param X Coordinate x, unit mm
 * @param Y Coordinate y, unit mm
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API jump_abs(const double X, const double Y);

/**
 * @brief Jump to absolute position
 * @note List Instruction
 * @param CardNo Board ID
 * @param X Coordinate x, unit mm
 * @param Y Coordinate y, unit mm
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_jump_abs(const uint32_t CardNo, const double X, const double Y);

/**
 * @brief Jump to relative position
 * @note List Instruction
 * @param dX
 * @param dY
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API jump_rel(const double dX, const double dY);

/**
 * @brief Jump to relative position
 * @note List Instruction
 * @param CardNo Board ID
 * @param dX Coordinate offset x, in mm
 * @param dY Coordinate offset y, in mm
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_jump_rel(const uint32_t CardNo, const double dX, const double dY);

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
LCS_IMPORT LCS2Error LCS_API set_offset_list(const uint32_t HeadNo, const double XOffset, const double YOffset, const uint32_t at_once);

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
LCS_IMPORT LCS2Error LCS_API n_set_offset_list(const uint32_t CardNo, const uint32_t HeadNo, const double XOffset, const double YOffset, const uint32_t at_once);

/**
 * @brief Set marking angle
 * @note List Instruction
 * @param HeadNo The galvanometer number is temporarily unavailable
 * @param Angle Rotation angle, counterclockwise is positive
 * @param at_once Is it only valid once, temporarily useless
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_angle_list(const uint32_t HeadNo, const double Angle, const uint32_t at_once);

/**
 * @brief Set marking angle
 * @note List Instruction
 * @param CardNo Board ID
 * @param HeadNo The galvanometer number is temporarily unavailable
 * @param Angle Rotation angle, counterclockwise is positive
 * @param at_once Is it only valid once, temporarily useless
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_angle_list(const uint32_t CardNo, const uint32_t HeadNo, const double Angle, const uint32_t at_once);

///////////////////////////////////////Galvanometer control (speed) related
/**
 * @brief Set marking speed
 * @note List Instruction
 * @param Speed Speed, unit mm/s
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_mark_speed(const double Speed);

/**
 * @brief Set marking speed
 * @note List Instruction
 * @param CardNo Board ID
 * @param Speed Speed, unit mm/s
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_mark_speed(const uint32_t CardNo, const double Speed);

/**
 * @brief Set jump speed
 * @note List Instruction
 * @param Speed Speed, unit mm/s
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_jump_speed(const double Speed);

/**
 * @brief Set jump speed
 * @note List Instruction
 * @param CardNo Board ID
 * @param Speed Speed, unit mm/s
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_jump_speed(const uint32_t CardNo, const double Speed);

/**
 * @brief Set galvanometer delay
 * @note List Instruction
 * @param Mark Marking delay, unit:us range:0~65535us
 * @param Polygon Corner delay, unit:us range:0~65535us
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_scanner_delays(const int32_t Mark, const int32_t Polygon);

/**
 * @brief Set galvanometer delay
 * @note List Instruction
 * @param CardNo Board ID
 * @param Mark Marking delay, unit:us range:0~65535us
 * @param Polygon Corner delay, unit:us range:0~65535us
 * @return  Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_scanner_delays(const uint32_t CardNo, const int32_t Mark, const int32_t Polygon);

/**
 * @brief Load calibration file
 * @note Control instruction
 * @param FileName Correct the full path of the file
 * @param bEnable Whether to enable calibration file
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API load_correction_file(const char* FileName, const bool bEnable);

/**
 * @brief Load calibration file
 * @note Control instruction
 * @param CardNo Board ID
 * @param FileName Correct the full path of the file
 * @param bEnable Whether to enable correction files
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_load_correction_file(const uint32_t CardNo, const char* FileName, const bool bEnable);

/**
 * @brief Enable galvanometer correction parameter file
 * @note Control instruction
 * @param bEnableA Whether to enable coordinate correction of galvanometer 1
 * @param bEnableB Whether to enable coordinate correction of galvanometer 2, temporarily useless
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API select_cor_table(const bool bEnableA, const bool bEnableB);

/**
 * @brief Enable galvanometer correction parameter file
 * @note Control instruction
 * @param CardNo Board ID
 * @param bEnableA Whether to enable coordinate correction of galvanometer 1
 * @param bEnableB Whether to enable coordinate correction of galvanometer 2, temporarily useless
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_select_cor_table(const uint32_t CardNo, const bool bEnableA, const bool bEnableB);

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
LCS_IMPORT LCS2Error LCS_API set_manual_correction_params(double scaleX, double scaleY, double bucketX, double bucketY, double paralleX, double paralleY, double trapeX, double trapeY);

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
LCS_IMPORT LCS2Error LCS_API n_set_manual_correction_params(const uint32_t CardNo, double scaleX, double scaleY, double bucketX, double bucketY, double paralleX, double paralleY, double trapeX, double trapeY);

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
LCS_IMPORT LCS2Error LCS_API set_scanahead_params(double fWorkSize, bool bXyFlip, bool bXInvert, bool bYInvert, double angle, double x_offset, double y_offset);

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
LCS_IMPORT LCS2Error LCS_API n_set_scanahead_params(const uint32_t CardNo, double fWorkSize, bool bXyFlip, bool bXInvert, bool bYInvert, double angle, double x_offset, double y_offset);

/**
 * @brief Clear marking count
 * @note Control instruction
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API clear_mark_count(void);

/**
 * @brief Clear marking count
 * @note Control instruction
 * @param CardNo Board ID
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_clear_mark_count(const uint32_t CardNo);

/**
 * @brief Obtain marking count
 * @note Control instruction
 * @return Marking count
 */
LCS_IMPORT uint32_t LCS_API get_mark_count(void);

/**
 * @brief Obtain marking count
 * @note Control instruction
 * @param CardNo Board ID
 * @return Marking count
 */
LCS_IMPORT uint32_t LCS_API n_get_mark_count(const uint32_t CardNo);

/**
 * @brief Marking times plus one
 * @note List Instruction
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API inc_mark_count(void);

/**
 * @brief Marking times plus one
 * @note List Instruction
 * @param CardNo Board ID
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_inc_mark_count(const uint32_t CardNo);

/**
 * @brief Set fiber/mopa power
 * @note List Instruction
 * @param power Power percentage, range 0~100
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API set_laser_power(unsigned char power);

/**
 * @brief Set fiber/mopa power
 * @note List Instruction
 * @param CardNo Board ID
 * @param power Power percentage, range 0~100
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_set_laser_power(const uint32_t CardNo, unsigned char power);

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
LCS_IMPORT LCS2Error LCS_API set_axis_move(int nAxisId, double fPulse, bool bOppDir, double RunSpeed, double startSpeed, uint16_t accTime);

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
LCS_IMPORT LCS2Error LCS_API n_set_axis_move(const uint32_t CardNo, int nAxisId, double fPulse, bool bOppDir, double RunSpeed, double startSpeed, uint16_t accTime);

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
LCS_IMPORT LCS2Error LCS_API set_axis_tozero(int nAxisId, double fPulse, bool bOppDir, double RunSpeed, double startSpeed, uint16_t accTime, bool nZeroType);

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
LCS_IMPORT LCS2Error LCS_API n_set_axis_tozero(const uint32_t CardNo, int nAxisId, double fPulse, bool bOppDir, double RunSpeed, double startSpeed, uint16_t accTime, bool nZeroType);

/**
* @brief Get Extension Axis Status
* @note Control instruction
* @param Status Return axis status, AxisStatus structure
* @param PosX Current position of the x-axis extension, unit:pulse defaults to 0x40000000
* @param PosY Current position of the y-axis extension, unit:pulse defaults to 0x40000000
* @return Error code
*/
LCS_IMPORT LCS2Error LCS_API get_axis_status(uint32_t* Status, uint32_t* PosX, uint32_t* PosY);

/**
* @brief Get Extension Axis Status
* @note Control instruction
* @param CardNo Board ID
* @param Status Return axis status, AxisStatus structure
* @param PosX Current position of the x-axis extension, unit:pulse defaults to 0x40000000
* @param PosY Current position of the y-axis extension, unit:pulse defaults to 0x40000000
* @return Error code
*/
LCS_IMPORT LCS2Error LCS_API n_get_axis_status(const uint32_t CardNo, uint32_t* Status, uint32_t* PosX, uint32_t* PosY);
/////////////////////////////////////////////The following is the API related to the network port card//////////////////////////////////////////////////////////////////////
/**
 * @brief Set card search timeout
 * @note Control instruction
 * @param TimeOut Timeout unit ms
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API eth_set_search_cards_timeout(const uint32_t TimeOut);

/**
 * @brief Count the number of network port cards
 * @note Control instruction
 * @return Number of boards
 */
LCS_IMPORT uint32_t LCS_API eth_count_cards(void);

/**
 * @brief Send a broadcast to find the network port card
 * @note Control instruction
 * @param Ip Local network port IP, network byte order
 * @param NetMask Find the subnet mask used for the network interface card (such as 255.255.255.255 or 192.168.0.255), network byte order
 * @return Number of boards found
 */
LCS_IMPORT uint32_t LCS_API eth_search_cards(const uint32_t Ip, const uint32_t NetMask);

/**
 * @brief Search for network interface cards within the specified IP segment
 * @note Control instruction
 * @param StartIp Starting IP, network byte order
 * @param EndIp End IP, network byte order
 * @return Number of boards found
 */
LCS_IMPORT uint32_t LCS_API eth_search_cards_range(const uint32_t StartIp, const uint32_t EndIp);

/**
 * @brief Get the network port card IP
 * @note Control instruction
 * @param CardNo Board ID
 * @return IP,Network byte order
 */
LCS_IMPORT uint32_t LCS_API eth_get_ip(const uint32_t CardNo);

/**
 * @brief Get the IP of the found card
 * @note Control instruction
 * @param SearchNo Found board serial number
 * @return IP,Network byte order
 */
LCS_IMPORT uint32_t LCS_API eth_get_ip_search(const uint32_t SearchNo);

/**
 * @brief Set static IP
 * @note Control instruction
 * @param Ip IP address, network byte order
 * @param NetMask Subnet mask, network byte order
 * @param Gateway Gateway address, network byte order
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API eth_set_static_ip(const uint32_t Ip, const uint32_t NetMask, const uint32_t Gateway);

/**
 * @brief Set static IP
 * @note Control instruction
 * @param CardNo Board ID
 * @param Ip IP address, network byte order
 * @param NetMask Subnet mask, network byte order
 * @param Gateway Gateway address, network byte order
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_eth_set_static_ip(const uint32_t CardNo, const uint32_t Ip, const uint32_t NetMask, const uint32_t Gateway);

/**
 * @brief Get static IP
 * @note Control instruction
 * @param Ip IP address, network byte order
 * @param NetMask Subnet mask, network byte order
 * @param Gateway Gateway address, network byte order
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API eth_get_static_ip(uint32_t* Ip, uint32_t* NetMask, uint32_t* Gateway);

/**
 * @brief Get static IP
 * @note Control instruction
 * @param CardNo Board ID
 * @param Ip IP address, network byte order
 * @param NetMask Subnet mask, network byte order
 * @param Gateway Gateway address, network byte order
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API n_eth_get_static_ip(const uint32_t CardNo, uint32_t* Ip, uint32_t* NetMask, uint32_t* Gateway);

/**
 * @brief Binding Card
 * @note Control instruction
 * @note Add the found board to the board manager
 * @param Ip IP address, network byte order
 * @param CardNo The Board ID bound to must be an ID that was not previously present in the Board Manager
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API eth_assign_card_ip(const uint32_t Ip, const uint32_t CardNo);

/**
 * @brief Binding UDP Card
 * @note Control instruction
 * @note Add the found board to the board manager
 * @param SearchNo Found board serial number
 * @param CardNo The Board ID bound to must be an ID that was not previously present in the Board Manager
 * @return Error code
 */
LCS_IMPORT LCS2Error LCS_API eth_assign_card(const uint32_t SearchNo, const uint32_t CardNo);

/**
* @brief Unbind UDP card
* @note Control instruction
* @note Remove the specified board from the Board Manager
* @param CardNo The bound board ID, must be an ID from the board manager
* @return Error code
*/
LCS_IMPORT LCS2Error LCS_API eth_remove_card(const uint32_t CardNo);

#if defined(__cplusplus)
}			//extern "C" 
#endif

