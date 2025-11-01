#include "pch.h"

#include "EmbedATK/Core/Logger.h"
#include "EmbedATK/Memory/Polymorphic.h"

#if !defined(EATK_DISABLE_LOGGING) && defined(__cpp_lib_format)

#if defined(EATK_PLATFORM_ARM)
constexpr size_t LOGGER_STACK_SIZE = 1024;
#else
constexpr size_t LOGGER_STACK_SIZE = 16384;
#endif
alignas(16) static std::array<std::byte, LOGGER_STACK_SIZE> g_loggerStackBuff;
std::span<std::byte> g_loggerStack = g_loggerStackBuff;

#if defined(EATK_PLATFORM_ARM)
constexpr size_t LOGGER_MSG_QUEUE_SIZE = 64;
#else
constexpr size_t LOGGER_MSG_QUEUE_SIZE = 1024;
#endif

using ConcreteLogger = std::tuple<Logger<LOGGER_MSG_QUEUE_SIZE>>;
static StaticPolymorphic<ILogger, ConcreteLogger> staticLogger;
ILogger* g_logger = nullptr;

void ILogger::init()
{
    staticLogger.construct<Logger<LOGGER_MSG_QUEUE_SIZE>>();
    g_logger = staticLogger.get();
}

void ILogger::shutdown()
{
    staticLogger.destroy();
    g_logger = nullptr;
}

#endif
