#include <stdio.h>
#include <string.h>
#include "cmsis_os.h"
#include "hal_trace.h"
#include "app_spp.h"
#include "spp_api.h"

#include "ai_spp.h"
#include "ai_manager.h"


/****************************************************************************
 * SPP SDP Entries
 ****************************************************************************/

/*---------------------------------------------------------------------------
 *
 * ServiceClassIDList 
 */
static const U8 SppClassId[] = {  
    SDP_ATTRIB_HEADER_8BIT(17),
    DETD_UUID + DESD_16BYTES,
    0x85,
    0xdb,\
    0xf2,\
    0xf9,\
    0x73,\
    0xe3,\
    0x43,\
    0xf5,\
    0xa1,\
    0x29,\
    0x97,\
    0x1b,\
    0x91,\
    0xc7,\
    0x2f,\
    0x1e,
};

static const U8 SppProtoDescList[] = {
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
    SDP_UINT_8BIT(RFCOMM_CHANNEL_TENCENT)
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
 * * OPTIONAL *  ServiceName
 */
static const U8 SppServiceName1[] = {
    SDP_TEXT_8BIT(5),          /* Null terminated text string */ 
    'S', 'p', 'p', '1', '\0'
};

static const U8 SppServiceName2[] = {
    SDP_TEXT_8BIT(5),          /* Null terminated text string */ 
    'S', 'p', 'p', '2', '\0'
};

/* SPP attributes.
 *
 * This is a ROM template for the RAM structure used to register the
 * SPP SDP record.
 */
static sdp_attribute_t sppSdpAttributes1[] = {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, SppClassId),

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, SppProtoDescList),

    SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, SppProfileDescList),

    /* SPP service name*/
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), SppServiceName1),
};

static sdp_attribute_t sppSdpAttributes2[] POSSIBLY_UNUSED= {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, SppClassId),

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, SppProtoDescList),

    SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, SppProfileDescList),

    /* SPP service name*/
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), SppServiceName2),
};

void TENCENT_SPP_Register128UuidSdpServices(uint8_t *service_id, btif_sdp_record_param_t *param)
{
    /* Register 128bit UUID SDP Attributes */
    *service_id = RFCOMM_CHANNEL_TENCENT;

    param->attrs = (sdp_attribute_t *)(&sppSdpAttributes1[0]);
    param->attr_count = ARRAY_SIZE(sppSdpAttributes1);
    param->COD = BTIF_COD_MAJOR_PERIPHERAL;

    TRACE(2,"%s serviceId %d", __func__, *service_id);
}

AI_SPP_TO_ADD(AI_SPEC_TENCENT, TENCENT_SPP_Register128UuidSdpServices);


