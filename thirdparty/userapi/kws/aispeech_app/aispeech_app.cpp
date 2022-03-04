#include "cmsis_os.h"
#include "cmsis.h"
#include "cqueue.h"
#include "hal_trace.h"
#include <string.h>
#include "aispeech.h"
#include "aispeech_app.h"

#include "app_utils.h"

// for audio
#include "audioflinger.h"
// #include "app_audio.h"
// #include "app_bt_stream.h"
// #include "app_media_player.h"

#define AISPEECH_AF_ID (AUD_STREAM_ID_0)

#define FRAME_LEN (256)
#define CHANNEL_NUM (1)
#define TEMP_BUF_SIZE (FRAME_LEN * CHANNEL_NUM * 2 * 2)

static uint8_t POSSIBLY_UNUSED codec_capture_buf[TEMP_BUF_SIZE];

#define WAKE_START_S(engine, env) wakeup_start(engine, (char *)env, strlen(env))

static wakeup_t *wakeup_engine;

extern bool start_by_sbc;

static int POSSIBLY_UNUSED ai_speech_wakeup_cb_handler(void *user_data, wakeup_status_t status, char *json, int bytes)
{
	TRACE(1,"%s\n", json);

	return 0;
}

int app_aispeech_process(short *pcm_buf, int pcm_len)
{
	wakeup_feed(wakeup_engine, (char *)pcm_buf, pcm_len * sizeof(short));

	return 0;
}

static uint32_t codec_capture_callback(uint8_t *buf, uint32_t len)
{
	short POSSIBLY_UNUSED *pcm_buf = (short *)buf;
	int pcm_len = len / 2;
	// TRACE(2,"[%s] cnt = %d", __func__, codec_capture_cnt++);
	// TRACE(2,"[%s] pcm_len = %d", __func__, pcm_len);

	// audio_dump_clear_up();

	// for (int i = 0; i < FRAME_LEN; i++)
	// {
	// 	mic0_pcm_buf[i] = pcm_buf[CHANNEL_NUM * i + 0];
	// 	mic1_pcm_buf[i] = pcm_buf[CHANNEL_NUM * i + 1];
	// }

	// audio_dump_add_channel_data(0, mic0_pcm_buf, FRAME_LEN);

	// audio_dump_add_channel_data(1, mic1_pcm_buf, FRAME_LEN);

	// audio_dump_run();

	app_aispeech_process(pcm_buf, FRAME_LEN);

	// speech_tx_process(pcm_buf, pcm_len, &pcm_buf, &pcm_len);

	// TRACE(0,"[test] trace manager!!!");

	// TRACE(0,"[audio dump] Test huoyj");
	// TRACE(0,"[audio dump] huoyj AUDIO_DUMP")

	return pcm_len * 2;
}

static int app_aispeech_init(bool on, void *param)
{
	int ret = 0;
	static bool isRun = false;
	enum APP_SYSFREQ_FREQ_T freq = APP_SYSFREQ_104M;
	struct AF_STREAM_CONFIG_T stream_cfg;

	TRACE(3,"[%s] isRun = %d, on = %d", __func__, isRun, on);

	if (isRun == on)
	{
		return 0;
	}

	if (on)
	{
		af_set_priority(AF_USER_THIRDPART, osPriorityHigh);
		freq = APP_SYSFREQ_104M;

		app_sysfreq_req(APP_SYSFREQ_USER_AI_VOICE, freq);
		TRACE(2,"[%s]: app_sysfreq_req %d", __func__, freq);
		//TRACE(1,"sys freq calc : %d\n", hal_sys_timer_calc_cpu_freq(5, 0));

		// app_overlay_select(APP_OVERLAY_HFP);

		// Initialize Cqueue

		// Initialize stream process
		// speech_init(tx_sample_rate, tx_frame_len, rx_sample_rate, rx_frame_len, speech_buf, speech_len);
		wakeup_engine = wakeup_new(NULL, NULL);
		wakeup_reset(wakeup_engine);
		wakeup_register_handler(wakeup_engine, NULL, ai_speech_wakeup_cb_handler);
		WAKE_START_S(wakeup_engine, "words=ni hao xiao le;thresh=0.10");
		wakeup_reset(wakeup_engine);

		// audio_dump_init(FRAME_LEN, sizeof(short), CHANNEL_NUM);

		ASSERT(CHANNEL_NUM == 1, "[%s] CHANNEL_NUM(%d) != 1", __func__, CHANNEL_NUM);

		memset(&stream_cfg, 0, sizeof(stream_cfg));
		stream_cfg.channel_num = (enum AUD_CHANNEL_NUM_T)CHANNEL_NUM;
		stream_cfg.data_size = TEMP_BUF_SIZE;
		stream_cfg.sample_rate = AUD_SAMPRATE_16000;
		stream_cfg.bits = AUD_BITS_16;
		stream_cfg.vol = 12;
		stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
		stream_cfg.io_path = AUD_INPUT_PATH_ASRMIC;
		stream_cfg.handler = codec_capture_callback;
		stream_cfg.data_ptr = codec_capture_buf;

		TRACE(2,"codec capture sample_rate:%d, data_size:%d", stream_cfg.sample_rate, stream_cfg.data_size);
		af_stream_open(AISPEECH_AF_ID, AUD_STREAM_CAPTURE, &stream_cfg);
		ASSERT(ret == 0, "codec capture failed: %d", ret);

#if defined(HW_DC_FILTER_WITH_IIR)
		hw_filter_codec_iir_st = hw_filter_codec_iir_create(stream_cfg.sample_rate, stream_cfg.channel_num, stream_cfg.bits, &adc_iir_cfg);
#endif

		if (!start_by_sbc)
		{
			TRACE(0,"audio capture on");
			af_stream_start(AISPEECH_AF_ID, AUD_STREAM_CAPTURE);
		}
	}
	else
	{
		// Close stream
		af_stream_stop(AISPEECH_AF_ID, AUD_STREAM_CAPTURE);

#if defined(HW_DC_FILTER_WITH_IIR)
		hw_filter_codec_iir_destroy(hw_filter_codec_iir_st);
#endif

		af_stream_close(AISPEECH_AF_ID, AUD_STREAM_CAPTURE);

		// deinitialize stream process
		wakeup_end(wakeup_engine);
		wakeup_delete(wakeup_engine);

		// speech_deinit();

		// app_overlay_unloadall();
		app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
		af_set_priority(AF_USER_THIRDPART, osPriorityAboveNormal);
	}

	isRun = on;
	return 0;
}

int app_aispeech_start(bool on, void *param)
{
	TRACE(2,"[%s] on = %d", __func__, on);
	// af_stream_stop(AISPEECH_AF_ID, AUD_STREAM_CAPTURE);
	if (on)
	{
		if (start_by_sbc)
		{
			af_stream_start(AISPEECH_AF_ID, AUD_STREAM_CAPTURE);
		}
	}
	//  CSpotterReset();
	//  damic_init();
	return 0;
}

// Just keep same with cyberon
int cspotter_get_page_efuse(unsigned char page, void *param)
{
	int efuse;
	if (page == 0XE)
	{
		efuse = 0x1B9C;
	}
	if (page == 0xF)
	{
		efuse = 0x098B;
	}
	return efuse;
}

#include "app_thirdparty.h"

// keep same with Cyberon
THIRDPARTY_EVENT_HANDLER_TAB_ADD(aispeech, AISPEECH_EVENT_NUM){
	{{THIRDPARTY_ID_NO1, THIRDPARTY_START}, (APP_THIRDPARTY_HANDLE_CB_T)app_aispeech_init, true, NULL},
	{{THIRDPARTY_ID_NO1, THIRDPARTY_STOP}, (APP_THIRDPARTY_HANDLE_CB_T)app_aispeech_init, false, NULL},
	{{THIRDPARTY_ID_NO1, THIRDPARTY_OTHER_EVENT}, (APP_THIRDPARTY_HANDLE_CB_T)app_aispeech_start, true, NULL},
	{{THIRDPARTY_ID_NO1, THIRDPARTY_GET_PAGE_E_EFUSE}, (APP_THIRDPARTY_HANDLE_CB_T)cspotter_get_page_efuse, 0XE, NULL},
	{{THIRDPARTY_ID_NO1, THIRDPARTY_GET_PAGE_F_EFUSE}, (APP_THIRDPARTY_HANDLE_CB_T)cspotter_get_page_efuse, 0XF, NULL},
};
