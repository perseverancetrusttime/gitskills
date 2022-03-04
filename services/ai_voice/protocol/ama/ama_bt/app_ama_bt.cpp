#include <stdio.h>
#include <string.h>
#include "cmsis_os.h"
#include "hal_trace.h"
#include "cqueue.h"
#include "app_spp.h"
#include "spp_api.h"
#include "sdp_api.h"

#include "ai_spp.h"
#include "ai_manager.h"
#include "ai_thread.h"
#include "app_dip.h"

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
#ifdef IOS_IAP2_PROTOCOL
    static const U8 IOS_AMA_SPP_128UuidClassId[] = {
        SDP_ATTRIB_HEADER_8BIT(17),        /* Data Element Sequence, 17 bytes */
        DETD_UUID + DESD_16BYTES,        /* Type & size index 0x1C */ \
        //0xFF, 0xCA, 0xCA, 0xDE, 0xAF, 0xDE, 0xCA, 0xDE, 0xDE, 0xFA, 0xCA, 0xDE, 0x00, 0x00, 0x00, 0x00  /* Bits[128:121] of UUID ios  */
        0x00, 0x00, 0x00, 0x00, 0xDE, 0xCA, 0xFA, 0xDE, 0xDE,0xCA, 0xDE, 0xAF, 0xDE, 0xCA, 0xCA, 0xFF
    };
#endif

static const U8 AMA_SPP_128UuidClassId[] = {
    SDP_ATTRIB_HEADER_8BIT(17),        /* Data Element Sequence, 17 bytes */
    DETD_UUID + DESD_16BYTES,        /* Type & size index 0x1C */ \
            0x93, /* Bits[128:121] of AMA UUID */ \
            0x1C, \
            0x7E, \
            0x8A, \
            0x54, \
            0x0F, \
            0x46, \
            0x86, \
            0xB7, \
            0x98, \
            0xE8, \
            0xDF, \
            0x0A, \
            0x2A, \
            0xD9, \
            0xF7,
};

static const U8 AMA_SPP_128UuidProtoDescList[] = {
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
    SDP_UINT_8BIT(RFCOMM_CHANNEL_AMA)
};

/*
 * * OPTIONAL * Language BaseId List (goes with the service name).  
 */ 
static const U8 AMA_SPP_128UuidLangBaseIdList[] = {
    SDP_ATTRIB_HEADER_8BIT(9),  /* Data Element Sequence, 9 bytes */ 
    SDP_UINT_16BIT(0x656E),     /* English Language */ 
    SDP_UINT_16BIT(0x006A),     /* UTF-8 encoding */ 
    SDP_UINT_16BIT(0x0100)      /* Primary language base Id */ 
};

/*
 * * OPTIONAL *  ServiceName
 */
static const U8 AMA_SPP_128UuidServiceName[] = {
    SDP_TEXT_8BIT(8),          /* Null terminated text string */ 
    'U', 'U', 'I', 'D', '1', '2', '8', '\0'
};

/* SPP attributes.
 *
 * This is a ROM template for the RAM structure used to register the
 * SPP SDP record.
 */
static sdp_attribute_t AMA_SPP_128UuidSdpAttributes[] = {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, AMA_SPP_128UuidClassId),

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, AMA_SPP_128UuidProtoDescList),

    /* Language base id (Optional: Used with service name) */ 
    SDP_ATTRIBUTE(AID_LANG_BASE_ID_LIST, AMA_SPP_128UuidLangBaseIdList),

    /* SPP service name*/
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), AMA_SPP_128UuidServiceName),
};

#ifdef IOS_IAP2_PROTOCOL
static sdp_attribute_t IOS_AMA_SPP_128UuidSdpAttributes[] = {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, IOS_AMA_SPP_128UuidClassId),

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, AMA_SPP_128UuidProtoDescList),

    /* Language base id (Optional: Used with service name) */ 
    SDP_ATTRIBUTE(AID_LANG_BASE_ID_LIST, AMA_SPP_128UuidLangBaseIdList),

    /* SPP service name*/
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), AMA_SPP_128UuidServiceName),
};
#endif

void AMA_SPP_Register128UuidSdpServices(uint8_t *service_id, btif_sdp_record_param_t *param)
{
    sdp_attribute_t *SPP_128UuidSdpAttributes = NULL;
    uint8_t sdp_record_num = 0;

    /* Register 128bit UUID SDP Attributes */
#ifdef IOS_IAP2_PROTOCOL
    if(app_ai_connect_mobile_has_ios())
    {
        SPP_128UuidSdpAttributes = IOS_AMA_SPP_128UuidSdpAttributes;
        sdp_record_num = ARRAY_SIZE(IOS_AMA_SPP_128UuidSdpAttributes);
    }
    else
#endif
    {
        SPP_128UuidSdpAttributes = AMA_SPP_128UuidSdpAttributes;
        sdp_record_num = ARRAY_SIZE(AMA_SPP_128UuidSdpAttributes);
    }

    /* Register 128bit UUID SDP Attributes */
    *service_id = RFCOMM_CHANNEL_AMA;

    param->attrs = SPP_128UuidSdpAttributes;
    param->attr_count = sdp_record_num;
    param->COD = BTIF_COD_MAJOR_PERIPHERAL;
    
    TRACE(2,"%s serviceId %d", __func__, *service_id);
}

AI_SPP_TO_ADD(AI_SPEC_AMA, AMA_SPP_Register128UuidSdpServices);

