/*
* Copyright © Shanghai Awinic Technology Co., Ltd. 2019 2019 . All rights reserved.
* Description: Communication protocol related header file
*/
#ifndef __AW_SOC_PROTOCOL_H
#define __AW_SOC_PROTOCOL_H
#include "aw_type.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#ifndef AW_OK
#define AW_OK   (0)
#endif

#ifndef AW_FAIL
#define AW_FAIL (1)
#endif


#define GET_3BYT(data)          (AW_U8)((data)>>24)
#define GET_2BYT(data)          (AW_U8)((data)>>16)
#define GET_1BYT(data)          (AW_U8)((data)>>8)
#define GET_0BYT(data)          (AW_U8)((data)>>0)

#define GET_16_DATA(x, y)       ((AW_U16)(((x) << 8) | (y)))
#define GET_HIGH_4BIT(data)     (((data) & 0xf0)>>4)
#define GET_LOW_4BIT(data)      ((data) & 0x0f)

#define PROTOCOL_TOTAL_LEN      (256)
#define PROTOCOL_L1_S_LEN       (sizeof(PROTOCOL_L1_S))
#define PROTOCOL_L2_S_LEN       (sizeof(PROTOCOL_L2_S))
#define IIC_L3_DATA_LEN         (PROTOCOL_TOTAL_LEN - PROTOCOL_L1_S_LEN - PROTOCOL_L2_S_LEN)
#define PROTOCOL_S_LEN          (sizeof(PROTOCOL_DATA_S))
#define CHECK_PROTOCOL_LEN      (PROTOCOL_L1_S_LEN + PROTOCOL_L2_S_LEN - 1)


#define CLEAN_HEAD_BYTE         ((AW_U32)0x00FFFFFF)
#define CHECK_ACK_LEN           (0x01)
#define HANDSHAKE_ACK_LEN       (0x05)
#define ID_NO_RESPOND_LEN       (0x04)
#define AW_LOCATION_UBOOT       ((AW_U32)0x00000001)
#define AW_FLASH_BOOT           ((AW_U32)0x00010002)
#define AW_RAM_BOOT             ((AW_U32)0x00010003)
#define READ_ACK_LENGTH         ((AW_U16)0x05) // flags and addresses

#define ERROR_LOCATION_DATA     (0)
#define FUN_LOCATION_DATA       (1)
#define WRITE_DATA_ADDR         (0)
#define WRITE_USE_DATA          (4)
#define LENGTH_DATA             (0)
#define LENGTH_DATA_H           (1)
#define READ_DATA_ADDR          (2)
#define READ_DATA_ADDR_ACK      (1)
#define READ_SEND_DATA          (5)
#define END_ADDR                (0)
#define ERR_ACK_MODULE          (1) // IF module or evnet id err ,ack moudel_id location length
#define ERR_ACK_EVNENT_LEN      (2) // IF module or evnet id err ,ack evenet_id location length
#define ERR_ACK_CHIP_LEN        (3) // IF module or evnet id err ,AW chip location length

#define ERASE_SCTOR_ID          (0x0321)
#define ERASE_CHIP_ID           (0x0323)

#define CONNECT_NUM             (0)
#define PROTOCOL_VERSION_NUM    (18)
#define VERSION_INFOR_NUM       (20)
#define CHIP_ID_NUM             (24)
#define CHIP_DATE_NUM           (26)
#define RAM_READ_NUM            (4)
#define FLASH_READ_NUM          (8)
#define UI_IRQ_NUM              (22)
#define DATA_LEN_H              (1)
#define DATA_LEN_L              (0)
#define DATA_ACK_LEN            (14)
#define DATA_ACK_ERR_LEN        (10)

#define UI_LENGTH_LEN           (2U)
#define NO_RESPOND_LEN          (3U)
#define UI_ADDR_LEN             (4U)
#define UI_L1_L2_LEN            (9U)

enum check_ack_enum
{
  AW_ACK_BUSY = 0x00,
  AW_ACK_FREE = 0x01,
};
typedef enum check_ack_enum AW_ACK_E;

enum version_enum
{
  AW_VERSION_1 = 0x01, // protocol version
  AW_VERSION_2 = 0x02,
  AW_VERSION_3 = 0x03,
};
typedef enum version_enum AW_VERSION_E;

enum id_enum
{
  PC_ADDRESS_ID = 0x01,
  MCU_ADDRESS_ID = 0x02,
  AP_ADDRESS_ID = 0x04,
  AW_ADDRESS_ID = 0x08,
};
typedef enum id_enum ADDRESS_ID_E;

enum module_enum
{
  AW_ID_FREE = 0X00,
  HANDSHAKE_ID = 0x01,
  RAM_ID = 0x02,
  FLASH_ID = 0x03,
  END_ID = 0x04,
  UI_IRQ_ID = 0x05,
  AW_MODULE_ID_ERR = 0Xff,
};
typedef enum module_enum MODULE_E;

enum connect_enum
{
  CONNECT_ID = 0x01,
  CONNECT_ACK_ID = 0x02,
  MOUDLE_EVNET_NO_RESPOND = 0x08,
  PROTOCOL_ID = 0x11,
  PROTOCOL_ACK_ID = 0x12,
  VERSION_ID = 0x21,
  VERSION_ACK_ID = 0x22,
  CHIP_ID = 0x31,
  CHIP_ID_ACK = 0x32,
  CHIP_DATE = 0x33,
  CHIP_DATE_ACK = 0x34,
  AW_EVENT_ID_ERR = 0Xff,
};
typedef enum connect_enum HANDSHAKE_EVENT_E;

enum ram_enum
{
  RAM_WRITE_ID = 0x01,
  RAM_WRITE_ACK_ID = 0x02,
  RAM_READ_ID = 0x11,
  RAM_READ_ACK_ID = 0x12,
};
typedef enum ram_enum RAM_EVENT_E;

enum flash_enum
{
  FLASH_WRITE_ID = 0x01,
  FLASH_WRITE_ACK_ID = 0x02,
  FLASH_READ_ID = 0x11,
  FLASH_READ_ACK_ID = 0x12,
  FLASH_ERASE_ID = 0x21,
  FLASH_ERASE_ACK_ID = 0x22,
  FLASH_ERASE_CHIP_ID = 0X23,
  FLASH_ERASE_CHIP_ACK_ID = 0X24,
};
typedef enum flash_enum FLASH_EVENT_E;

enum end_enum
{
  RAM_JUMP_ID = 0x01,
  RAM_JUMP_ACK_ID = 0x02,
  FLASH_JUMP_ID = 0x11,
  FLASH_JUMP_ACK_ID = 0x12,
  ROM_JUMP_ID = 0x21,
  ROM_JUMP_ACK_ID = 0x22,
};
typedef enum end_enum END_EVENT_E;

enum ui_irq_enum
{
  READ_INT_STA_ID = 0x01,
  READ_INT_STA_ACK_ID = 0x02,
};
typedef enum ui_irq_enum UI_IRQ_E;

enum check_flag_enum
{
  AW_FLAG_OK = 0x00,
  AW_FLAG_FAIL = 0x01,
  AW_CHECK_HEADER_ERR = 0x02,
  AW_VERSION_ERR = 0x03,
  AW_ADDRESS_ID_ERR = 0x04,
  AW_CHECK_DATA_ERR = 0x05,
  AW_CHECK_ID_ERR = 0x06,
  AW_ADDRESS_ERR = 0x07,
  AW_HANDSHAKE_ERR = 0x10,
  AW_RAM_WRITE_ERR = 0x20,
  AW_RAM_READ_ERR = 0x21,
  AW_FLASH_WRITE_ERR = 0x30,
  AW_FLASH_READ_ERR = 0x31,
  AW_FLASH_SECTOR_ERR = 0x32,
  AW_FLASH_CHIP_ERR = 0x33,
  AW_END_ERR = 0x40,
  AW_TIME_OUT_ERR = 0x41,
  AW_EXCEPTION_ERR = 0xff,
};
typedef enum check_flag_enum AW_CHECK_FLAG_E;


struct protocol_l1_struct{
  AW_U8 check_header;
  AW_U8 version;
  AW_U8 adress;
};
typedef struct protocol_l1_struct PROTOCOL_L1_S;

struct protocol_l2_struct{
  AW_U8 module_id;
  AW_U8 event_id;
  AW_U8 payload_length_l;
  AW_U8 payload_length_h;
  AW_U8 ack;
  AW_U8 check_data;
};
typedef struct protocol_l2_struct PROTOCOL_L2_S;

struct protocol_data_struct{
  PROTOCOL_L1_S protocol_l1_s; //
  PROTOCOL_L2_S protocol_l2_s;
  AW_U8 protocol_l3_data[];
};
typedef struct protocol_data_struct PROTOCOL_DATA_S;

struct data_buff_struct{
  AW_U8 dest_adr;
  AW_U8 src_adr;
};
typedef struct data_buff_struct COM_DATA_BUFF_S;

struct gui_to_soc_struct{
  AW_U16 version_num; // Effective data length
  AW_U16 soc_data_len; // Effective data length
  AW_U16 ui_rd_data_len; // Data length to be read after the upper computer finishes writing data
  AW_U16 read_len; // Read addr data length
  AW_U32 addr; // Address where writes data
  AW_U8 device_commu;
  AW_U8 device_addr;
  AW_U8 dest_adr; // Destination address means: to whom?
  AW_U8 src_adr; // source addres means:form where?
  AW_U8 module_id; // module id
  AW_U8 event_id; // evnet id
  AW_U8 err_flag; // Operation status of lower computer
  AW_U8 reserved0; // reserved
  AW_U8 reserved1; // reserved
  AW_U8 soc_data[100]; // Effective data
};
typedef struct gui_to_soc_struct GUI_TO_SOC_S;


#ifdef __cplusplus
}
#endif /* __cplusplus */



AW_U8 aw_soc_protocol_pack_interface(GUI_TO_SOC_S *p_gui_data_s,
				     AW_U8 *p_aw_protocol_tx_data);
AW_U8 aw_soc_protocol_unpack_interface(GUI_TO_SOC_S *p_gui_data_s,
				       AW_U8 *p_aw_protocol_rx_data);

#endif


