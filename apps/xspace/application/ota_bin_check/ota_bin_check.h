#ifndef __OTA_BIN_CHECK_H__
#define __OTA_BIN_CHECK_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__APP_BIN_INTEGRALITY_CHECK__)
/**
 ****************************************************************************************
 * @brief check recevied ota_bin'shmac_sha256
 *
 * @note compare the ori hmac sha256 with caculate hmac_sha256 key
 *       ori hmac sha256 :store in the end of ota_bin -keyword was "sha"
 *
 * @param[in] _4k_pool                  pointer which point 4k buffer space
 * @param[in] image_flash_offset        NEW_IMAGE_FLASH_OFFSET
 * @param[in] image_size                new image size
 *
 * @return  success return true ; otherwise:false
 ****************************************************************************************
 */
bool ota_bin_hmac_sha256_check(uint8_t *_4k_pool, uint32_t image_flash_offset, uint32_t image_size);
#endif

void app_update_magic_number_of_app_image(uint8_t *_4k_pool, uint32_t newMagicNumber);

#ifdef __COMPRESSED_IMAGE__
/**
 ****************************************************************************************
 * @brief check compressed image header/footer 
 *
 * @note compressed image header/footer  which was assigned by xz-internal crc32
 *      
 * @param[in] srcFlashOffset        NEW_IMAGE_FLASH_OFFSET
 * @param[in] image_size                new image size
 *
 * @return  success return true ; otherwise:false
 ****************************************************************************************
 */
bool app_check_compressed_image(uint32_t srcFlashOffset, uint32_t imageSize);

/**
 ****************************************************************************************
 * @brief check compress_image was depressed success
 *
 * @note check compress_image was depressed success by xz decompress process which was embeded 
 *       in our devices
 *      
 * @param[in] srcFlashOffset        NEW_IMAGE_FLASH_OFFSET
 * @param[in] xz_mem_pool           pointer which point to fixed 1024*160 heap size
 * @param[in] xz_mem_pool_size      fixed_length at heap seciton -1024 * 160 
 * 
 * @return  success return true ; otherwise:false
 ****************************************************************************************
 */
bool app_check_compressed_image2(uint32_t srcFlashOffset, uint8_t *xz_mem_pool, uint32_t xz_mem_pool_size);

/**
 ****************************************************************************************
 * @brief decompressd new_app_image and check hmac_sha256 image
 *
 * @note first:decompress new_app_image from NEW_IMAGE_FLASH_OFFSET->APP_IMAGE_OFFSET
 *       second: enter APP_IMAGE_OFFSET and check app_image hmac_sha256
 *       final: reboot to normal boot
 *       
 * @param[in] srcFlashOffset        NEW_IMAGE_FLASH_OFFSET
 * @param[in] dstFlashOffset        __APP_IMAGE_FLASH_OFFSET__
 * @param[in] xz_mem_pool           pointer which point to fixed 1024*160 heap size
 * @param[in] xz_mem_pool_size      fixed_length at heap seciton -1024 * 160 
 * 
 * @return  success                    return 1;
 *          decompress failed          return 2;
 *          hamc_sha256 check faile    return 3;
 ****************************************************************************************
 */
uint8_t app_copy_compressed_image(uint32_t srcFlashOffset, uint32_t dstFlashOffset, uint8_t *xz_mem_pool, uint32_t xz_mem_pool_size);
#endif

#ifdef __cplusplus
}
#endif
#endif   //__OTA_BIN_CHECK_H__