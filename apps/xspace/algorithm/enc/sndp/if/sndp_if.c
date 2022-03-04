#ifdef __SP_TRIP_MIC__
#include "BT_DMSpxEnh_lib.h"
#include "sndp_if.h"

int SndpEc_TxSpxEnh_MemBufSizeInBytes_if(void){
	return SndpEc_TxSpxEnh_MemBufSizeInBytes();
}
unsigned char *sndp_get_lib_version_if(void){
	return sndp_get_lib_version();
}
int sndp_license_status_get_if(void){
	return sndp_license_status_get();
}
unsigned char *sndp_get_lib_built_datetime_if(void){
	return sndp_get_lib_built_datetime();
}
void SndpEc_TxSpxEnh_Init_if(void *membuf,int NrwFlag){
	SndpEc_TxSpxEnh_Init(membuf,NrwFlag);
}

int sndp_license_auth_if(uint8_t* license_key, int Key_len){
	return sndp_license_auth(license_key,Key_len);
}
void SndpEC_Tx_MMicSpxEhnFrame_if(float* out, float* AecedX, float* echos, float* pMic_In, int AncFlag){
	return SndpEC_Tx_MMicSpxEhnFrame(out,AecedX,echos,pMic_In,AncFlag);
}
void SndpEC_Tx_AecFrame_if(float *out, float *out_echos, float *pMic_In, float *pRef_In){
	SndpEC_Tx_AecFrame(out, out_echos, pMic_In, pRef_In);
}
void Sndp_SpxEnh_Rx_if(float *outY, float *inX, int numsamples, int AncFlag){
}
#endif

