#include "pch.h"

#include "EmbedATK/OSAL/OSAL.h"

#if defined(EATK_PLATFORM_WINDOWS)
    #include "../../platform/windows/osal/WindowsOSAL.h"
#elif defined(EATK_PLATFORM_LINUX)
    #include "../../platform/linux/osal/LinuxOSAL.h"
#elif defined(EATK_PLATFORM_ARM)
    #include "../../platform/arm/osal/ArmOSAL.h"
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