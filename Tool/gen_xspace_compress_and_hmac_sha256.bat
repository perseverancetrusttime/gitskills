@echo off
::生成用于OTA的bin文件

set bin_name=xspace_ibrt

:: STEP 01: 复制%bin_name%.bin到当前目录下
copy /Y ..\out\%bin_name%\%bin_name%.bin %bin_name%.bin

:: STEP 02: %bin_name%.bin 添加sha256摘要信息
call xspace_hmac_sha256.exe %bin_name%.bin

:: STEP 03: %bin_name%_sha256.bin 进行压缩。
call compress_utils.exe -z -f -k %bin_name%_sha256.bin

:: STEP 04: 对压缩后文件添加sha256摘要信息，生成%bin_name%_sha256_sha256.bin
call xspace_hmac_sha256.exe %bin_name%_sha256.bin.xzip

:: STEP 04: 改名
copy %bin_name%_sha256_sha256.bin %bin_name%_for_ota.bin

:: STEP 05：删除中间文件
del %bin_name%.bin 
del %bin_name%_sha256.bin
del %bin_name%_sha256.bin.xzip
del %bin_name%_sha256_sha256.bin

:: %bin_name%_sha256_sha256.bin用于OTA

pause