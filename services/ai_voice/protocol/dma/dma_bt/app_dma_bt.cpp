#include <stdio.h>
#include <string.h>
#include "cmsis_os.h"
#include "hal_trace.h"
#include "cqueue.h"
#include "app_spp.h"
#include "spp_api.h"

#include "ai_spp.h"
#include "ai_manager.h"
#include "app_dma_bt.h"


/****************************************************************************
 * SPP SDP Entries
 ****************************************************************************/

/*---------------------------------------------------------------------------
 *
 * ServiceClassIDList 
 */


/* Extend UUID */
/*---------------------------------------------------------------------------
 *
 * ServiceClassIDList
 */
//#define IOS_IAP2_PROTOCOL //for test

static const U8 DMA_SPP_128UuidClassId[] = {
    SDP_ATTRIB_HEADER_8BIT(17),        /* Data Element Sequence, 17 bytes */
    DETD_UUID + DESD_16BYTES,        /* Type & size index 0x1C */ \
            0x51, /* Bits[128:121] of DMA UUID */ \
            0xDB, \
            0xA1, \
            0x09, \
            0x5B, \
            0xA9, \
            0x49, \
            0x81, \
            0x96, \
            0xB7, \
            0x6A, \
            0xFE, \
            0x13, \
            0x20, \
            0x93, \
            0xDE,
};


static const U8 DMA_SPP_128UuidProtoDescList[] = {
    SDP_ATTRIB_HEADER_8BIT(12),  /* Data element sequence, 12 bytes */ 

    /* Each element of the list is a Protocol descriptor which is a 
     * data element sequence. The first element is L2CAP which only 
     * has a UUID element.  
     */ 
    SDP_ATTRIB_HEADER_8BIT(3),   /* Data element sequence for L2CAP, 3 
                                  * bytes 
                                  */ 

    SDP_UUID_16BIT(PROT_L2CAP),  /* Uuid16 L2CAP */ 

    /* Next protocol descriptor in the list is RFCOMM. It contains two 
     * elements which are the UUID and the channel. Ultimately this 
     * channel will need to filled in with value returned by RFCOMM.  
     */ 

    /* Data element sequence for RFCOMM, 5 bytes */
    SDP_ATTRIB_HEADER_8BIT(5),

    SDP_UUID_16BIT(PROT_RFCOMM), /* Uuid16 RFCOMM */

    /* Uint8 RFCOMM channel number - value can vary */
    SDP_UINT_8BIT(RFCOMM_CHANNEL_BAIDU)
};

/*
 * BluetoothProfileDescriptorList
 */
static const U8 SppProfileDescList[] = {
    SDP_ATTRIB_HEADER_8BIT(8),        /* Data element sequence, 8 bytes */

    /* Data element sequence for ProfileDescriptor, 6 bytes */
    SDP_ATTRIB_HEADER_8BIT(6),

    SDP_UUID_16BIT(SC_SERIAL_PORT),   /* Uuid16 SPP */
    SDP_UINT_16BIT(0x0102)            /* As per errata 2239 */ 
};

/*
 * * OPTIONAL * Language BaseId List (goes with the service name).  
 */ 
static const U8 DMA_SPP_128UuidLangBaseIdList[] = {
    SDP_ATTRIB_HEADER_8BIT(9),  /* Data Element Sequence, 9 bytes */ 
    SDP_UINT_16BIT(0x656E),     /* English Language */ 
    SDP_UINT_16BIT(0x006A),     /* UTF-8 encoding */ 
    SDP_UINT_16BIT(0x0100)      /* Primary language base Id */ 
};

/*
 * * OPTIONAL *  ServiceName
 */
static const U8 DMA_SPP_128UuidServiceName[] = {
    SDP_TEXT_8BIT(8),          /* Null terminated text string */ 
    'U', 'U', 'I', 'D', '1', '2', '8', '\0'
};

/* SPP attributes.
 *
 * This is a ROM template for the RAM structure used to register the
 * SPP SDP record.
 */
static sdp_attribute_t DMA_SPP_128UuidSdpAttributes[] = {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, DMA_SPP_128UuidClassId),

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, DMA_SPP_128UuidProtoDescList),

    SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, SppProfileDescList),
    ///* Language base id (Optional: Used with service name) */ 
    //SDP_ATTRIBUTE(AID_LANG_BASE_ID_LIST, DMA_RFCOMM_128UuidLangBaseIdList),

    /* SPP service name*/
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), DMA_SPP_128UuidServiceName),
};

void DMA_SPP_Register128UuidSdpServices(uint8_t *service_id, btif_sdp_record_param_t *param)
{
    /* Register 128bit UUID SDP Attributes */
    *service_id = RFCOMM_CHANNEL_BAIDU;

    param->attrs = (sdp_attribute_t *)(&DMA_SPP_128UuidSdpAttributes[0]);
    param->attr_count = ARRAY_SIZE(DMA_SPP_128UuidSdpAttributes);
    param->COD = BTIF_COD_MAJOR_PERIPHERAL;

    TRACE(2,"%s serviceId %d", __func__, *service_id);
}

AI_SPP_TO_ADD(AI_SPEC_BAIDU, DMA_SPP_Register128UuidSdpServices);


