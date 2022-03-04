/***************************************************************************************************
 *Name: aw_log.h
 *Created on: 2020.4.19
 *Author: www.awinic.com.cn
 ***************************************************************************************************/
#ifndef _AW_LOG_H_
#define _AW_LOG_H_
#include <stdint.h>
#include <stdarg.h>
#include "hal_trace.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  LOG_NONE = 0,
  LOG_ERROR,
  LOG_WARN,
  LOG_INFO,
  LOG_DEBUG,
  LOG_VERBOSE,
} aw_log_level_t;

#ifndef AW_LOG_LEVEL
#define AW_LOG_LEVEL  INFO
#endif

#define LOG_COLOR_BLACK "30"
#define LOG_COLOR_RED "31"
#define LOG_COLOR_GREEN "32"
#define LOG_COLOR_BROWN "33"
#define LOG_COLOR_BLUE "34"
#define LOG_COLOR_PURPLE "35"
#define LOG_COLOR_CYAN "36"
#define LOG_COLOR_WHITE "37"

#define LOG_COLOR(COLOR) "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR) "\033[1;" COLOR "m"
#define LOG_RESET_COLOR "\033[0m"

#define LOG_COLOR_E LOG_COLOR(LOG_COLOR_RED)
#define LOG_COLOR_W LOG_COLOR(LOG_COLOR_BROWN)
#define LOG_COLOR_I LOG_COLOR(LOG_COLOR_GREEN)
#define LOG_COLOR_D LOG_COLOR(LOG_COLOR_BLUE)
#define LOG_COLOR_V LOG_COLOR(LOG_COLOR_BLACK)

//#define AW_LOG_TIMETAMP          (uint32_t)AW_TIM_getRunTime()
#define AW_LOG_TIMETAMP            0
#define AW_CFG_LOG_LOCAL_LEVEL     LOG_VERBOSE
#define AW_LOG_FORMAT(format)   "[%s]" format 

//#define AW_LOG_OUT(format,...) printf(format,##__VA_ARGS__)
#define AW_LOG_OUT(num, format,...) TRACE(num+1,format,##__VA_ARGS__)

#define AW_LOG_EARLY_IMPL(num, tag, format, log_level, ...) do { \
  if (AW_CFG_LOG_LOCAL_LEVEL >= log_level) { \
      AW_LOG_OUT(num, AW_LOG_FORMAT(format), tag, ##__VA_ARGS__); \
}} while(0)

#define AW_EARLY_LOGE(num, tag, format, ... ) AW_LOG_EARLY_IMPL(num, tag, format, LOG_ERROR, ##__VA_ARGS__)
#define AW_EARLY_LOGW(num, tag, format, ... ) AW_LOG_EARLY_IMPL(num, tag, format, LOG_WARN, ##__VA_ARGS__)
#define AW_EARLY_LOGI(num, tag, format, ... ) AW_LOG_EARLY_IMPL(num, tag, format, LOG_INFO, ##__VA_ARGS__)
#define AW_EARLY_LOGD(num, tag, format, ... ) AW_LOG_EARLY_IMPL(num, tag, format, LOG_DEBUG, ##__VA_ARGS__)
#define AW_EARLY_LOGV(num, tag, format, ... ) AW_LOG_EARLY_IMPL(num, tag, format, LOG_VERBOSE, ##__VA_ARGS__)

#define AW_LOGE(num, tag, format, ... ) AW_EARLY_LOGE(num, tag, format, ##__VA_ARGS__)
#define AW_LOGW(num, tag, format, ... ) AW_EARLY_LOGW(num, tag, format, ##__VA_ARGS__)
#define AW_LOGI(num, tag, format, ... ) AW_EARLY_LOGI(num, tag, format, ##__VA_ARGS__)
#define AW_LOGD(num, tag, format, ... ) AW_EARLY_LOGD(num, tag, format, ##__VA_ARGS__)
#define AW_LOGV(num, tag, format, ... ) AW_EARLY_LOGV(num, tag, format, ##__VA_ARGS__)



#ifdef __cplusplus
}
#endif

#endif
