#include "pch.h"

#include "EmbedATK/Core/Logger.h"
#include "EmbedATK/Memory/Polymorphic.h"

#if !defined(EATK_DISABLE_LOGGING) && defined(__cpp_lib_format)

#if defined(EATK_PLATFORM_ARM)
constexpr size_t LOGGER_MSG_QUEUE_SIZE = 16;
#else
constexpr size_t LOGGER_MSG_QUEUE_SIZE = 1024;
#endif

#if defined(EATK_PLATFORM_ARM)
constexpr size_t LOGGER_STACK_SIZE = 8*1024;
#else
constexpr size_t LOGGER_STACK_SIZE = 16384;
#endif

using ConcreteLogger = Logger<LOGGER_MSG_QUEUE_SIZE, LOGGER_STACK_SIZE>;
static StaticPolymorphic<ILogger, ConcreteLogger> s_logger;
ILogger* g_logger = nullptr;

void ILogger::init(int prio)
{
    s_logger.construct<Logger<LOGGER_MSG_QUEUE_SIZE, LOGGER_STACK_SIZE>>(prio);
    g_logger = s_logger.get();
    s_logger.as<ConcreteLogger>()->start();
}

void ILogger::shutdown()
{
    s_logger.destroy();
    g_logger = nullptr;
}

#endif
