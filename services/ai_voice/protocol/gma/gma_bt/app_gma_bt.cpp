#ifdef __GMA_VOICE__

#include <stdio.h>
#include <string.h>
#include "cmsis_os.h"
#include "hal_trace.h"
#include "cqueue.h"
#include "app_spp.h"
#include "spp_api.h"

#include "ai_spp.h"
#include "ai_manager.h"

#define  UUID_128_ENABLE
/****************************************************************************
 * SPP SDP Entries
 ****************************************************************************/
/*---------------------------------------------------------------------------
 *
 * ServiceClassIDList 
 */
static const U8 GMA_SPP_ClassId[] = {
    SDP_ATTRIB_HEADER_8BIT(3),        /* Data Element Sequence, 6 bytes */ 
    SDP_UUID_16BIT(SC_SERIAL_PORT),     /* Hands-Free UUID in Big Endian */ 
};

static const U8 GMA_SPP_ProtoDescList[] = {
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
    SDP_UINT_8BIT(RFCOMM_CHANNEL_ALI)
};

/*
 * BluetoothProfileDescriptorList
 */
static const U8 GMA_SPP_ProfileDescList[] = {
    SDP_ATTRIB_HEADER_8BIT(8),        /* Data element sequence, 8 bytes */

    /* Data element sequence for ProfileDescriptor, 6 bytes */
    SDP_ATTRIB_HEADER_8BIT(6),

    SDP_UUID_16BIT(SC_SERIAL_PORT),   /* Uuid16 SPP */
    SDP_UINT_16BIT(0x0102)            /* As per errata 2239 */ 
};

/*
 * * OPTIONAL *  ServiceName
 */
static const U8 GMA_SPP_ServiceName1[] = {
    SDP_TEXT_8BIT(5),          /* Null terminated text string */ 
    'S', 'p', 'p', '1', '\0'
};

static const U8 GMA_SPP_ServiceName2[] = {
    SDP_TEXT_8BIT(5),          /* Null terminated text string */ 
    'S', 'p', 'p', '2', '\0'
};

/* SPP attributes.
 *
 * This is a ROM template for the RAM structure used to register the
 * SPP SDP record.
 */
static sdp_attribute_t GMA_SPP_SdpAttributes1[] = {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, GMA_SPP_ClassId), 

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, GMA_SPP_ProtoDescList),

    SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, GMA_SPP_ProfileDescList),

    /* SPP service name*/
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), GMA_SPP_ServiceName1),
};

#if 0
static sdp_attribute_t GMA_SPP_SdpAttributes2[] = {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, GMA_SPP_ClassId), 

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, GMA_SPP_ProtoDescList),

    SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, GMA_SPP_ProfileDescList),

    /* SPP service name*/
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), GMA_SPP_ServiceName2),
};
#endif

void GMA_SPP_RegisterSdpServices(uint8_t *service_id, btif_sdp_record_param_t *param)
{
    /* Register 128bit UUID SDP Attributes */
    *service_id = RFCOMM_CHANNEL_ALI;

    param->attrs = (sdp_attribute_t *)(&GMA_SPP_SdpAttributes1[0]);
    param->attr_count = ARRAY_SIZE(GMA_SPP_SdpAttributes1);
    param->COD = BTIF_COD_MAJOR_PERIPHERAL;

    TRACE(2,"%s serviceId %d", __func__, *service_id);
}

#ifdef UUID_128_ENABLE
/* Extend UUID */
uint8_t UUID_128[16] = {0x6a, 0x29, 0x80, 0x85, 0x50, 0x86, 0x55, 0xab, 0x89, 0x47, 0xf4, 0x57, 0xf3, 0x0a, 0x3e, 0xeb};
//uint8_t UUID_128[16] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x01, 0x11, 0x00, 0x00};
/*---------------------------------------------------------------------------
 *
 * ServiceClassIDList
 */
static const U8 GMA_SPP_128UuidClassId[] = {
    SDP_ATTRIB_HEADER_8BIT(17),        /* Data Element Sequence, 17 bytes */
    SDP_UUID_128BIT(UUID_128),    /* 128 bit UUID in Big Endian */
};

static const U8 GMA_SPP_128UuidProtoDescList[] = {
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
    SDP_UINT_8BIT(RFCOMM_CHANNEL_ALI)
};

/*
 * * OPTIONAL * Language BaseId List (goes with the service name).  
 */ 
static const U8 GMA_SPP_128UuidLangBaseIdList[] = {
    SDP_ATTRIB_HEADER_8BIT(9),  /* Data Element Sequence, 9 bytes */ 
    SDP_UINT_16BIT(0x656E),     /* English Language */ 
    SDP_UINT_16BIT(0x006A),     /* UTF-8 encoding */ 
    SDP_UINT_16BIT(0x0100)      /* Primary language base Id */ 
};

/*
 * * OPTIONAL *  ServiceName
 */
static const U8 GMA_SPP_128UuidServiceName[] = {
    SDP_TEXT_8BIT(8),          /* Null terminated text string */ 
    'U', 'U', 'I', 'D', '1', '2', '8', '\0'
};

/* SPP attributes.
 *
 * This is a ROM template for the RAM structure used to register the
 * SPP SDP record.
 */
static sdp_attribute_t GMA_SPP_128UuidSdpAttributes[] = {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, GMA_SPP_128UuidClassId),

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, GMA_SPP_128UuidProtoDescList),

    /* Language base id (Optional: Used with service name) */ 
    SDP_ATTRIBUTE(AID_LANG_BASE_ID_LIST, GMA_SPP_128UuidLangBaseIdList),

    /* SPP service name*/
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), GMA_SPP_128UuidServiceName),
};

void GMA_SPP_Register128UuidSdpServices(uint8_t *service_id, btif_sdp_record_param_t *param)
{
    /* Register 128bit UUID SDP Attributes */
    *service_id = RFCOMM_CHANNEL_ALI;

    param->attrs = (sdp_attribute_t *)(&GMA_SPP_128UuidSdpAttributes[0]);
    param->attr_count = ARRAY_SIZE(GMA_SPP_128UuidSdpAttributes);
    param->COD = BTIF_COD_MAJOR_PERIPHERAL;

    TRACE(2,"%s serviceId %d", __func__, *service_id);
}

#endif

#ifdef UUID_128_ENABLE
    AI_SPP_TO_ADD(AI_SPEC_ALI, GMA_SPP_Register128UuidSdpServices);
#else
    AI_SPP_TO_ADD(AI_SPEC_ALI, GMA_SPP_RegisterSdpServices);
#endif

#endif

