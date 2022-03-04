#ifndef __SNDP_IF_ADAPTER_H__
#define __SNDP_IF_ADAPTER_H__

#ifdef __cplusplus
extern "C" {
#endif
int SndpEc_TxSpxEnh_MemBufSizeInBytes_if(void);
unsigned char *sndp_get_lib_version_if(void);
unsigned char *sndp_get_lib_built_datetime_if(void);
void SndpEc_TxSpxEnh_Init_if(void *membuf,int NrwFlag);
int sndp_license_status_get_if(void);
int sndp_license_auth_if(uint8_t* license_key, int Key_len);
void SndpEC_Tx_MMicSpxEhnFrame_if(float* out, float* AecedX, float* echos, float* pMic_In, int AncFlag);
void SndpEC_Tx_AecFrame_if(float *out, float *out_echos, float *pMic_In, float *pRef_In);
void Sndp_SpxEnh_Rx_if(float *outY, float *inX, int numsamples, int AncFlag);
#ifdef __cplusplus
}
#endif
#endif	

