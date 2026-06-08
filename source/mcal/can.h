#ifndef CAN_H
#define CAN_H



/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>


/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/


#define GRUPO_NUM 4
#define MY_ID (0x100 + GRUPO_NUM) // 0x104

#define CAN_BAUDRATE    125000U     // 125 kHz
#define TOTAL_TQ    16U         // Cuántos Tq sumaste (1+3+6+6)


/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/



/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/


/**
 * @brief Initializes the CAN0 module
 * @param 
 * @return false if initialized succesfully
 */
bool CAN0_Init(void);

/**
 * @brief Function to transmit a message
 * @param id id of the message
 * @param data pointer to data of the message
 * @param size number of bytes of the message
 * @return false if buffer was already full, true if message was saved on the buffer
 */
bool CAN0_WriteMessage(uint32_t id, uint8_t* data, uint8_t size);

/**
 * @brief Getter 
 * @param id pointer where id should be saved
 * @param data pointer to where data should be saved
 * @return false if there is no message, true if a message was received
 */
bool CAN0_MsgGetter(uint32_t* id, uint8_t* data, uint8_t *size);






#endif
