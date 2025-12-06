set(THREADX_ARCH cortex_m7)
set(THREADX_TOOLCHAIN gnu)
set(TX_ENABLE_STACK_CHECKING ON)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../external/eclipse_threadx/threadx)
set(NXD_ENABLE_FILE_SERVERS OFF)
set(NXD_ENABLE_BSD ON)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../external/eclipse_threadx/netxduo)

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
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/lan8742/lan8742.c

        ${CMAKE_CURRENT_LIST_DIR}/../../external/eclipse_threadx/netxduo/common/drivers/ethernet/nx_stm32_eth_driver.c
        ${CMAKE_CURRENT_LIST_DIR}/../../external/eclipse_threadx/netxduo/common/drivers/ethernet/lan8742/nx_stm32_phy_driver.c
)

target_include_directories(Platform 
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Inc
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-hal-driver/Inc/Legacy
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/stm32h7xx-nucleo-bsp
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/cmsis-core/Include
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/cmsis-device-h7/Include
        ${CMAKE_CURRENT_LIST_DIR}/../../external/stm/lan8742

        ${CMAKE_CURRENT_LIST_DIR}/../../external/eclipse_threadx/netxduo/common/drivers/ethernet
        ${CMAKE_CURRENT_LIST_DIR}/../../../../../CubeMX/NetXDuo/Target/
)

target_compile_definitions(Platform 
    INTERFACE
        USE_PWR_LDO_SUPPLY
        USE_NUCLEO_64
        USE_HAL_DRIVER
        STM32H753xx
        $<$<CONFIG:Debug>:DEBUG>

        TX_INCLUDE_USER_DEFINE_FILE
        NX_INCLUDE_USER_DEFINE_FILE
        NX_BSD_RAW_SUPPORT
        NX_DISABLE_BSD_RAW_PACKET_DUPLICATE
        NX_BSD_PRINT_ERRORS
)

target_link_libraries(Platform
    INTERFACE
        threadx
        netxduo
)
