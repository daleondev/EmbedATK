set(TARGET_FLAGS "-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard ")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -MMD -MP")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fdata-sections -ffunction-sections")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-Os -g0")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -g0")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -frtti -fno-threadsafe-statics -fexceptions")

add_library(Platform INTERFACE)

target_sources(Platform
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_tim.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_tim_ex.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_cortex.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_eth.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_eth_ex.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_rcc.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_rcc_ex.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_flash.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_flash_ex.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_gpio.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_hsem.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_dma.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_dma_ex.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_mdma.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_pwr.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_pwr_ex.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_i2c.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_i2c_ex.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_exti.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_rng.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_rng_ex.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_rtc.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_rtc_ex.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_usart.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_usart_ex.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_uart_ex.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Src/stm32h7xx_hal_uart.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-nucleo-bsp/stm32h7xx_nucleo.c
)

target_include_directories(Platform 
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Inc
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Inc/Legacy
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-nucleo-bsp
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/cmsis-core/Include
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/cmsis-device-h7/Include
)

target_compile_definitions(Platform 
    INTERFACE
        USE_PWR_LDO_SUPPLY
        USE_NUCLEO_64
        USE_HAL_DRIVER
        STM32H753xx
        $<$<CONFIG:Debug>:DEBUG>
)

# add_library(Platform INTERFACE)
# target_include_directories(Platform 
#     INTERFACE
#         ${CMAKE_CURRENT_LIST_DIR}/include
#         ${CMAKE_CURRENT_SOURCE_DIR}/external/stm/stm32h7xx-hal-driver/Inc
#         ${CMAKE_CURRENT_SOURCE_DIR}/external/stm/cmsis-core/Core/Include
#         ${CMAKE_CURRENT_SOURCE_DIR}/external/stm/cmsis-device-h7/Include
#         ${CMAKE_CURRENT_SOURCE_DIR}/external/eclipse_threadx/threadx/common/inc
#         ${CMAKE_CURRENT_SOURCE_DIR}/external/eclipse_threadx/threadx/ports/cortex_m7/gnu/inc
#         ${CMAKE_CURRENT_SOURCE_DIR}/external/eclipse_threadx/netxduo/common/inc
#         ${CMAKE_CURRENT_SOURCE_DIR}/external/eclipse_threadx/netxduo/ports/cortex_m7/gnu/inc
# )
# target_include_directories(Platform 
#     SYSTEM 
#     INTERFACE
#         ${CMAKE_CURRENT_SOURCE_DIR}/external/eclipse_threadx/netxduo/addons/BSD
# )
# target_compile_definitions(Platform 
#     INTERFACE
#         USE_PWR_LDO_SUPPLY 
#         USE_NUCLEO_64 
#         TX_INCLUDE_USER_DEFINE_FILE 
#         NX_INCLUDE_USER_DEFINE_FILE 
#         USE_HAL_DRIVER 
#         STM32H753xx
#         $<$<CONFIG:Debug>:DEBUG>

#         NX_BSD_ENABLE_NATIVE_API
#         NX_BSD_RAW_SUPPORT
#         NX_DISABLE_BSD_RAW_PACKET_DUPLICATE
#         NX_BSD_PRINT_ERRORS
# )