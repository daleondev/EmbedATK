set(TARGET_FLAGS "-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard ")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -MMD -MP")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fdata-sections -ffunction-sections")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-Os -g0")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -g0")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -frtti -fno-threadsafe-statics -fexceptions")

set(THREADX_ARCH cortex_m7)
set(THREADX_TOOLCHAIN gnu)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/eclipse_threadx/threadx)

add_library(Platform INTERFACE)
target_link_libraries(Platform 
    INTERFACE
        threadx
)
# target_include_directories(Platform 
#     SYSTEM 
#     INTERFACE
#         ${CMAKE_CURRENT_LIST_DIR}/Middlewares/ST/netxduo/addons/BSD
# )
target_compile_definitions(Platform 
    INTERFACE
        USE_PWR_LDO_SUPPLY 
        USE_NUCLEO_64 
        # TX_INCLUDE_USER_DEFINE_FILE 
        # NX_INCLUDE_USER_DEFINE_FILE 
        USE_HAL_DRIVER 
        STM32H753xx
        $<$<CONFIG:Debug>:DEBUG>

        NX_BSD_ENABLE_NATIVE_API
        NX_BSD_RAW_SUPPORT
        NX_DISABLE_BSD_RAW_PACKET_DUPLICATE
        NX_BSD_PRINT_ERRORS
)