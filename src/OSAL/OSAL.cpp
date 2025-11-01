// #include "pch.h"

// #include "EmbedATK/OSAL/OSAL.h"

// #if defined(EATK_PLATFORM_WINDOWS)
//     #include "windows/src/OSAL/WindowsOSAL.h"
// #elif defined(EATK_PLATFORM_LINUX)
//     #include "linux/src/OSAL/LinuxOSAL.h"
// #elif defined(EATK_PLATFORM_ARM)
//     #include "threadx_arm/src/OSAL/ArmOSAL.h"
// #else
//     #error "Unsupported Platform"
// #endif

// const OSAL& OSAL::instance()
// {
// #if defined(EATK_PLATFORM_WINDOWS)
//     static const WindowsOSAL s_instance;
// #elif defined(EATK_PLATFORM_LINUX)
//     static const LinuxOSAL s_instance;
// #elif defined(EATK_PLATFORM_ARM)
//     static const ArmOSAL s_instance;
// #endif
//     return s_instance;
// }

// template <typename T, size_t N>
// void OSAL::createMessageQueue(IPolymorphic<OSAL::MessageQueue<T, N>>& queue)
// {
//     using ImplType = typename MessageQueueImpl<T, N>::Type;
//     queue.template construct<ImplType>(); 
// }

// // #include <format>
// // #if defined(EATK_DISABLE_LOGGING) && defined(__cpp_lib_format)
// // #include "KitCAT/Core/Logger.h"
// // template void OSAL::createMessageQueue(IPolymorphic<OSAL::MessageQueue<PmrUniquePtr<ILogger::LogData>, 1024>>&);
// // #endif

// // #include "Ecat/EcatIO.h"
// // template void OSAL::createMessageQueue(IPolymorphic<OSAL::MessageQueue<EcatIO::IOReq, EcatIO::MSG_QUEUE_SIZE>>&);
// // template void OSAL::createMessageQueue(IPolymorphic<OSAL::MessageQueue<EcatIO::IOResp, EcatIO::MSG_QUEUE_SIZE>>&);