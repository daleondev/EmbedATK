#include "pch.h"

#include "EmbedATK/OSAL/OSAL.h"

#if defined(EATK_PLATFORM_WINDOWS)
    #include "windows/src/OSAL/WindowsOSAL.h"
#elif defined(EATK_PLATFORM_LINUX)
    #include "linux/src/OSAL/LinuxOSAL.h"
#elif defined(EATK_PLATFORM_ARM)
    #include "arm/src/OSAL/ArmOSAL.h"
#else
    #error "Unsupported Platform"
#endif

const OSAL& OSAL::instance()
{
#if defined(EATK_PLATFORM_WINDOWS)
    static const WindowsOSAL s_instance;
#elif defined(EATK_PLATFORM_LINUX)
    static const LinuxOSAL s_instance;
#elif defined(EATK_PLATFORM_ARM)
    static const ArmOSAL s_instance;
#endif
    return s_instance;
}