/*
* Copyright © Shanghai Awinic Technology Co., Ltd. 2019 2020 . All rights reserved.
* Description: Communication protocol fun related header file
*/
#include "aw_soc_protocol.h"


static COM_DATA_BUFF_S g_aw_com_data;

static COM_DATA_BUFF_S *aw_get_com_data_struct(void)
{
  return &g_aw_com_data;
}

static AW_U8 aw_set_u32_fun(AW_U8 *u8_addr, AW_U32 u32_data)
{
  u8_addr[0] = GET_0BYT(u32_data);
  u8_addr[1] = GET_1BYT(u32_data);
  u8_addr[2] = GET_2BYT(u32_data);
  u8_addr[3] = GET_3BYT(u32_data);

  return AW_OK;
}

static AW_U32 aw_get_u32_fun(AW_U8 *u8_addr)
{
  return (AW_U32)((u8_addr[0]<<0) + (u8_addr[1]<<8) +
                  (u8_addr[2]<<16) + (u8_addr[3]<<24));
}

static AW_U8 check_sum(AW_U8 *buf, AW_U16 len)
{
  AW_U8 sum_data = 0;
  AW_U16 i = 0;

  for (i = 0; i < len; i++) {
    sum_data += buf[i];
  }

  return sum_data;
}

static const AW_U16 id_table[] = {
  GET_16_DATA(HANDSHAKE_ID, CONNECT_ID),GET_16_DATA(HANDSHAKE_ID, CONNECT_ACK_ID),//0,1  0x0101,0x0102
  GET_16_DATA(RAM_ID, RAM_WRITE_ID),GET_16_DATA(RAM_ID, RAM_WRITE_ACK_ID),//2,3  0x0201,0x0202
  GET_16_DATA(RAM_ID, RAM_READ_ID),GET_16_DATA(RAM_ID, RAM_READ_ACK_ID),//4,5  0x0211,0x0212
  GET_16_DATA(FLASH_ID, FLASH_WRITE_ID),GET_16_DATA(FLASH_ID, FLASH_WRITE_ACK_ID),//6,7  0x0301,0x0302
  GET_16_DATA(FLASH_ID, FLASH_READ_ID),GET_16_DATA(FLASH_ID, FLASH_READ_ACK_ID),//8,9  0x0311,0x0312
  GET_16_DATA(FLASH_ID, FLASH_ERASE_ID),GET_16_DATA(FLASH_ID, FLASH_ERASE_ACK_ID),//10,11  0x0321.0x0322
  GET_16_DATA(FLASH_ID, FLASH_ERASE_CHIP_ID),GET_16_DATA(FLASH_ID, FLASH_ERASE_CHIP_ACK_ID),//12,13  0x0323,0x0324
  GET_16_DATA(END_ID, RAM_JUMP_ID),GET_16_DATA(END_ID, RAM_JUMP_ACK_ID),//14,15  0x0401,0x0402
  GET_16_DATA(END_ID, FLASH_JUMP_ID),GET_16_DATA(END_ID, FLASH_JUMP_ACK_ID),//16,17  0x0411,0x0412
  GET_16_DATA(HANDSHAKE_ID, PROTOCOL_ID),GET_16_DATA(HANDSHAKE_ID, PROTOCOL_ACK_ID),//18,19  0x0111,0x0112
  GET_16_DATA(HANDSHAKE_ID, VERSION_ID),GET_16_DATA(HANDSHAKE_ID, VERSION_ACK_ID),//20,21  0x0121,0x0122
  GET_16_DATA(UI_IRQ_ID, READ_INT_STA_ID),GET_16_DATA(UI_IRQ_ID, READ_INT_STA_ACK_ID),//22,23  0x0501,0x0502
  GET_16_DATA(HANDSHAKE_ID, CHIP_ID),GET_16_DATA(HANDSHAKE_ID, CHIP_ID_ACK),//24,25  0x0131,0x0132
  GET_16_DATA(HANDSHAKE_ID, CHIP_DATE),GET_16_DATA(HANDSHAKE_ID, CHIP_DATE_ACK),//26,27  0x0133,0x0134
  GET_16_DATA(AW_ID_FREE, AW_ID_FREE),GET_16_DATA(HANDSHAKE_ID, MOUDLE_EVNET_NO_RESPOND),//28,29  0x0000,0x0108
  GET_16_DATA(END_ID, ROM_JUMP_ID),GET_16_DATA(END_ID, ROM_JUMP_ACK_ID),//30,31  0x0421,0x0422
};

static AW_U8 aw_check_id_fun(AW_U8 module_id, AW_U8 event_id)
{
  AW_U8 i = 0;
  AW_U8 i_max = 0;
  AW_U16 id_num = 0;

  i_max = sizeof(id_table) / sizeof(AW_U16);
  id_num = GET_16_DATA(module_id, event_id);
  for (i = 0; i < i_max; i++) {
    if (id_num == id_table[i]) {
      return AW_OK;
    }
  }
  return AW_FAIL;
}

static AW_U8 aw_parsing_protocol_header(PROTOCOL_DATA_S *p_aw_protocol_rx_data_s)
{
  COM_DATA_BUFF_S *p_aw_com_data = AW_NULL;

  p_aw_com_data = aw_get_com_data_struct();
  p_aw_com_data->dest_adr = GET_HIGH_4BIT(p_aw_protocol_rx_data_s->protocol_l1_s.adress);
  p_aw_com_data->src_adr = GET_LOW_4BIT(p_aw_protocol_rx_data_s->protocol_l1_s.adress);

  return AW_OK;
}


static AW_U8 aw_set_ui_read_data_len(GUI_TO_SOC_S *p_gui_data_s)
{
  AW_U16 id_num = 0;

  id_num = GET_16_DATA(p_gui_data_s->module_id, p_gui_data_s->event_id);

  if ((id_num == id_table[CONNECT_NUM]) ||
      (id_num == id_table[PROTOCOL_VERSION_NUM]) ||
      (id_num == id_table[VERSION_INFOR_NUM]) ||
      (id_num == id_table[CHIP_ID_NUM]) ||
      (id_num == id_table[CHIP_DATE_NUM])) {
    p_gui_data_s->ui_rd_data_len = DATA_ACK_LEN;
  } else if ((id_num == id_table[RAM_READ_NUM]) ||
             (id_num == id_table[FLASH_READ_NUM]) ||
             (id_num == id_table[UI_IRQ_NUM])) {
    p_gui_data_s->ui_rd_data_len = (p_gui_data_s->read_len + DATA_ACK_LEN);
  } else {
    p_gui_data_s->ui_rd_data_len = DATA_ACK_ERR_LEN;
  }

  return AW_OK;
}


static AW_U8 aw_set_protocol_header_data(PROTOCOL_DATA_S *p_aw_protocol_tx_data_s, AW_U16 data_len)
{
  AW_U8 *p_protocol_tx_l3 = AW_NULL;
  PROTOCOL_L1_S *p_protocol_tx_l1_s = AW_NULL;
  PROTOCOL_L2_S *p_protocol_tx_l2_s = AW_NULL;
  COM_DATA_BUFF_S *p_aw_com_data = AW_NULL;

  p_protocol_tx_l3 = p_aw_protocol_tx_data_s->protocol_l3_data;
  p_protocol_tx_l1_s = &p_aw_protocol_tx_data_s->protocol_l1_s;
  p_protocol_tx_l2_s = &p_aw_protocol_tx_data_s->protocol_l2_s;
  p_aw_com_data = aw_get_com_data_struct();

  p_protocol_tx_l1_s->version = AW_VERSION_1;
  p_protocol_tx_l1_s->adress = (AW_U8)(p_aw_com_data->dest_adr | p_aw_com_data->src_adr<<4);
  p_protocol_tx_l2_s->payload_length_l = GET_0BYT(data_len);
  p_protocol_tx_l2_s->payload_length_h = GET_1BYT(data_len);
  p_protocol_tx_l2_s->ack = AW_ACK_FREE;
  p_protocol_tx_l2_s->check_data = check_sum(p_protocol_tx_l3, data_len);
  p_protocol_tx_l1_s->check_header = check_sum(&(p_protocol_tx_l1_s->version), CHECK_PROTOCOL_LEN);

  return AW_OK;
}


static AW_CHECK_FLAG_E check_protocol_header_data(PROTOCOL_DATA_S *p_aw_protocol_rx_data_s, ADDRESS_ID_E aw_id_e)
{
  AW_U8 check_sum_data = 0;
  AW_U8 check_id_num = 0;
  AW_U16 payload_length_data = 0;
  AW_U8 *p_protocol_rx_l3 = AW_NULL;
  PROTOCOL_L1_S *p_protocol_rx_l1_s = AW_NULL;
  PROTOCOL_L2_S *p_protocol_rx_l2_s = AW_NULL;
  COM_DATA_BUFF_S *p_aw_com_data = AW_NULL;

  p_protocol_rx_l3 = p_aw_protocol_rx_data_s->protocol_l3_data;
  p_protocol_rx_l1_s = &p_aw_protocol_rx_data_s->protocol_l1_s;
  p_protocol_rx_l2_s = &p_aw_protocol_rx_data_s->protocol_l2_s;
  p_aw_com_data = aw_get_com_data_struct();

  aw_parsing_protocol_header(p_aw_protocol_rx_data_s);

  payload_length_data = GET_16_DATA(p_protocol_rx_l2_s->payload_length_h,
                                    p_protocol_rx_l2_s->payload_length_l);

  check_id_num = aw_check_id_fun(p_protocol_rx_l2_s->module_id, p_protocol_rx_l2_s->event_id);
  check_sum_data = check_sum(&(p_protocol_rx_l1_s->version), CHECK_PROTOCOL_LEN);
  if (check_sum_data != p_protocol_rx_l1_s->check_header) {
    return AW_CHECK_HEADER_ERR;
  } else if (p_protocol_rx_l1_s->version != AW_VERSION_1) {
    return AW_VERSION_ERR;
  } else if (p_aw_com_data->dest_adr != aw_id_e) {
    return AW_ADDRESS_ID_ERR;
  } else if (check_id_num == AW_FAIL) {
    return AW_CHECK_ID_ERR;
  } else {
    check_sum_data = check_sum(p_protocol_rx_l3, payload_length_data);
    if ((check_sum_data != p_protocol_rx_l2_s->check_data)) {
      return AW_CHECK_DATA_ERR;
    }
  }

  return AW_FLAG_OK;
}


/**
  * @brief  L3 layer conversion function from UI data to SOC data
  * @param  p_aw_protocol_data_s,p_aw_protocol_data_s,data_len
  * @retval AW_U8 data
  */
static AW_U8 aw_gui_exchange_buff(PROTOCOL_DATA_S *p_aw_protocol_data_s,
                                  GUI_TO_SOC_S *p_gui_data_s,
                                  AW_U16 *data_len)
{
  AW_U16 i = 0;

  /* If there is data length in L3 layer */
  if (p_gui_data_s->read_len != 0) {
    /* Populate data according to protocol */
    p_aw_protocol_data_s->protocol_l3_data[LENGTH_DATA] = GET_0BYT(p_gui_data_s->read_len);
    p_aw_protocol_data_s->protocol_l3_data[LENGTH_DATA_H] = GET_1BYT(p_gui_data_s->read_len);
    /* Set L3 layer data */
    aw_set_u32_fun(&p_aw_protocol_data_s->protocol_l3_data[READ_DATA_ADDR], p_gui_data_s->addr);
    /* Calculate L3 data length */
    *data_len = UI_LENGTH_LEN + UI_ADDR_LEN;
    } else {
      if (p_gui_data_s->soc_data_len == 0) {
        if (p_gui_data_s->addr != 0) {
          /* When there is only address, the data length of L3 layer is the address length */
          aw_set_u32_fun(p_aw_protocol_data_s->protocol_l3_data, p_gui_data_s->addr);
          *data_len = UI_ADDR_LEN;
        } else {
          /* When both data and address are 0, L3 layer length is 0 */
          *data_len = p_gui_data_s->soc_data_len;
        }
      } else {
        /* Set L3 layer data */
        aw_set_u32_fun(p_aw_protocol_data_s->protocol_l3_data, p_gui_data_s->addr);
        for (i = 0; i < p_gui_data_s->soc_data_len; i++) {
          p_aw_protocol_data_s->protocol_l3_data[i + UI_ADDR_LEN] = p_gui_data_s->soc_data[i];
        }
        /* Set L3 layer data length */
        *data_len = p_gui_data_s->soc_data_len + UI_ADDR_LEN;
      }
    }

  return AW_OK;
}

/**
  * @brief  L3 layer data extraction function
  * @param  p_gui_data_s,p_aw_protocol_tx_data,data_len
  * @retval AW_U8 data
  */
static AW_U8 aw_soc_exchange_buff(GUI_TO_SOC_S *p_gui_data_s,
                                  PROTOCOL_DATA_S *p_aw_protocol_data_s,
                                  AW_U16 data_len)
{
  AW_U16 i = 0;
  AW_U32 ui_data = 0;

  if (data_len > IIC_L3_DATA_LEN) {
    data_len = IIC_L3_DATA_LEN;
  }

  if (data_len == CHECK_ACK_LEN) {
    /* No useful data */
    p_gui_data_s->soc_data_len = 0;
  } else if (data_len == ID_NO_RESPOND_LEN) { // Data in handshake module
    /* set data length */
    p_gui_data_s->soc_data_len = NO_RESPOND_LEN;
    /* Put the data into the UI structure */
    p_gui_data_s->soc_data[0] = p_aw_protocol_data_s->protocol_l3_data[ERR_ACK_MODULE];
    p_gui_data_s->soc_data[1] = p_aw_protocol_data_s->protocol_l3_data[ERR_ACK_EVNENT_LEN];
    p_gui_data_s->soc_data[2] = p_aw_protocol_data_s->protocol_l3_data[ERR_ACK_CHIP_LEN];
  } else if (data_len == HANDSHAKE_ACK_LEN) { // Data in handshake module
    /* set data length */
    p_gui_data_s->soc_data_len = UI_ADDR_LEN;
    /* Put the data into the UI structure */
    ui_data = aw_get_u32_fun(&p_aw_protocol_data_s->protocol_l3_data[FUN_LOCATION_DATA]);
    aw_set_u32_fun(p_gui_data_s->soc_data, ui_data);
  } else if (data_len >= READ_ACK_LENGTH) {
    /* Subtract the length of address and flag */
    data_len = data_len - READ_ACK_LENGTH;
    /* set data length */
    p_gui_data_s->soc_data_len = data_len;
    /* set addr */
    p_gui_data_s->addr = aw_get_u32_fun(&p_aw_protocol_data_s->protocol_l3_data[READ_DATA_ADDR_ACK]);
    /* set data */
    for (i = 0; i < data_len; i++) {
      p_gui_data_s->soc_data[i] = p_aw_protocol_data_s->protocol_l3_data[i + READ_ACK_LENGTH];
    }
  } else {
    return AW_FAIL;
  }

  return AW_OK;
}


/**
  * @brief  UI to SOC packet function
  * @param  p_gui_data_s,p_aw_protocol_tx_data
  * @retval AW_U8 data
  */
static AW_U8 aw_soc_protocol_pack(GUI_TO_SOC_S *p_gui_data_s,
                           AW_U8 *p_aw_protocol_tx_data)
{
  AW_U16 aw_data_len = 0;
  PROTOCOL_DATA_S *p_aw_protocol_tx_data_s = AW_NULL;
  PROTOCOL_L2_S *p_protocol_tx_l2_s = AW_NULL;
  COM_DATA_BUFF_S *p_aw_com_data = AW_NULL;

  /* Strongly convert array to struct type */
  p_aw_protocol_tx_data_s = (PROTOCOL_DATA_S *)p_aw_protocol_tx_data;
  p_protocol_tx_l2_s = &p_aw_protocol_tx_data_s->protocol_l2_s;

  /* set dest addr and src addr */
  p_aw_com_data = aw_get_com_data_struct();
  p_aw_com_data->dest_adr = p_gui_data_s->src_adr;
  p_aw_com_data->src_adr = p_gui_data_s->dest_adr;

  /* set module and event id */
  p_protocol_tx_l2_s->module_id = p_gui_data_s->module_id;
  p_protocol_tx_l2_s->event_id = p_gui_data_s->event_id;

  /* Get UI read IIC ack data length */
  aw_set_ui_read_data_len(p_gui_data_s);
  /* L3 layer conversion function from UI data to SOC data */
  aw_gui_exchange_buff(p_aw_protocol_tx_data_s, p_gui_data_s, &aw_data_len);
  /* Calculate the current total data length */
  p_gui_data_s->soc_data_len = aw_data_len + UI_L1_L2_LEN;
  /* Data check and combination */
  aw_set_protocol_header_data(p_aw_protocol_tx_data_s, aw_data_len);

  return AW_OK;
}


/**
  * @brief  SOC to UI data conversion function
  * @param  p_gui_data_s,p_aw_protocol_tx_data,aw_id_e
  * @retval AW_CHECK_FLAG_E flag
  */
static AW_CHECK_FLAG_E aw_soc_protoco_unpack(GUI_TO_SOC_S *p_gui_data_s,
                                      AW_U8 *p_aw_protocol_rx_data)
{
  AW_U16 aw_data_len = 0;
  AW_CHECK_FLAG_E sta_flag = AW_FLAG_OK;
  PROTOCOL_DATA_S *p_aw_protocol_rx_data_s = AW_NULL;
  PROTOCOL_L2_S *p_protocol_rx_l2_s = AW_NULL;

  /* Strongly convert array to struct type */
  p_aw_protocol_rx_data_s = (PROTOCOL_DATA_S *)p_aw_protocol_rx_data;
  p_protocol_rx_l2_s = &p_aw_protocol_rx_data_s->protocol_l2_s;
  /* get data length */
  aw_data_len = GET_16_DATA(p_protocol_rx_l2_s->payload_length_h,
								p_protocol_rx_l2_s->payload_length_l);
  /* L3 layer data extraction */
  aw_soc_exchange_buff(p_gui_data_s, p_aw_protocol_rx_data_s, aw_data_len);
  p_gui_data_s->module_id = p_protocol_rx_l2_s->module_id;
  p_gui_data_s->event_id = p_protocol_rx_l2_s->event_id;
  p_gui_data_s->err_flag = p_aw_protocol_rx_data_s->protocol_l3_data[ERROR_LOCATION_DATA];
  p_gui_data_s->version_num = AW_VERSION_1;
  /* Check data */

  sta_flag = check_protocol_header_data(p_aw_protocol_rx_data_s, (ADDRESS_ID_E)p_gui_data_s->src_adr);

  return sta_flag;
}


static void aw_set_pack_fixed_data(GUI_TO_SOC_S *p_gui_data_s)
{
  p_gui_data_s->version_num = 0x0001; //consistent
  p_gui_data_s->ui_rd_data_len = 0x0000; //Fixed when packaged and sent
  p_gui_data_s->device_commu = 0x01; //UI use
  p_gui_data_s->device_addr = 0x5c; //i2c device addr
  p_gui_data_s->dest_adr = 0x08;	// Destination address means: to whom?
  p_gui_data_s->src_adr = 0x01;	// source addres means:form where?
  p_gui_data_s->err_flag = 0x00;	// Operation status of lower computer
  p_gui_data_s->reserved0 = 0x00;	// reserved0
  p_gui_data_s->reserved1 = 0x00;	// reserved1
}

AW_U8 aw_soc_protocol_pack_interface(GUI_TO_SOC_S *p_gui_data_s,
                                     AW_U8 *p_aw_protocol_tx_data)
{
  aw_set_pack_fixed_data(p_gui_data_s);
  return aw_soc_protocol_pack(p_gui_data_s, p_aw_protocol_tx_data);
}

AW_U8 aw_soc_protocol_unpack_interface(GUI_TO_SOC_S *p_gui_data_s,
                                       AW_U8 *p_aw_protocol_rx_data)
{
  return aw_soc_protoco_unpack(p_gui_data_s, p_aw_protocol_rx_data);
}





