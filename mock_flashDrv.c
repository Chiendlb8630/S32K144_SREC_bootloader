#include "mcu_mock.h"
#include <string.h>

flash_ssd_config_t flashSSDConfig;
status_t mock_flash_init(void){
    status_t status;
    memset(&flashSSDConfig, 0, sizeof(flash_ssd_config_t));
    status = FLASH_DRV_Init(&Flash_InitConfig0, &flashSSDConfig);
    return status;
}
