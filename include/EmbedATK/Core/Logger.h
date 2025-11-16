#pragma once

#include "EmbedATK/OSAL/OSAL.h"

#include "EmbedATK/Container/Queue.h"

#include "EmbedATK/Utils/MessageQueue.h"
#include "EmbedATK/Utils/Thread.h"

#include <format>
#include <source_location>

#if !defined(EATK_DISABLE_LOGGING) && defined(__cpp_lib_format)

    #include <regex>

    #define EATK_INIT_LOG(prio)                 ILogger::init(prio)
    #define EATK_SHUTDOWN_LOG()                 ILogger::shutdown()

    #define EATK_TRACE_LOC(loc, fmt, ...)       g_logger->trace(true, loc, fmt, ##__VA_ARGS__)
    #define EATK_INFO_LOC(loc, fmt, ...)        g_logger->info(true, loc, fmt, ##__VA_ARGS__)
    #define EATK_WARN_LOC(loc, fmt, ...)        g_logger->warn(true, loc, fmt, ##__VA_ARGS__)
    #define EATK_ERROR_LOC(loc, fmt, ...)       g_logger->error(true, loc, fmt, ##__VA_ARGS__)
    #define EATK_FATAL_LOC(loc, fmt, ...)       g_logger->fatal(true, loc, fmt, ##__VA_ARGS__)
    #define EATK_HIGHLIGHT_LOC(loc, fmt, ...)   g_logger->highlight(true, loc, fmt, ##__VA_ARGS__)

    #define EATK_TRACE(fmt, ...)		        g_logger->trace(true, METHOD_NAME, fmt, ##__VA_ARGS__)
    #define EATK_INFO(fmt, ...)		            g_logger->info(true, METHOD_NAME, fmt, ##__VA_ARGS__)
    #define EATK_WARN(fmt, ...)		            g_logger->warn(true, METHOD_NAME, fmt, ##__VA_ARGS__)
    #define EATK_ERROR(fmt, ...)		        g_logger->error(true, METHOD_NAME, fmt, ##__VA_ARGS__)
    #define EATK_FATAL(fmt, ...)		        g_logger->fatal(true, METHOD_NAME, fmt, ##__VA_ARGS__)
    #define EATK_HIGHLIGHT(fmt, ...)		    g_logger->highlight(true, METHOD_NAME, fmt, ##__VA_ARGS__)

    #define EATK_TRACE_NOW(fmt, ...)	        g_logger->trace(false, METHOD_NAME, fmt, ##__VA_ARGS__)
    #define EATK_INFO_NOW(fmt, ...)	            g_logger->info(false, METHOD_NAME, fmt, ##__VA_ARGS__)
    #define EATK_WARN_NOW(fmt, ...)	            g_logger->warn(false, METHOD_NAME, fmt, ##__VA_ARGS__)
    #define EATK_ERROR_NOW(fmt, ...)	        g_logger->error(false, METHOD_NAME, fmt, ##__VA_ARGS__)
    #define EATK_FATAL_NOW(fmt, ...)	        g_logger->fatal(false, METHOD_NAME, fmt, ##__VA_ARGS__)
    #define EATK_HIGHLIGHT_NOW(fmt, ...)	    g_logger->highlight(false, METHOD_NAME, fmt, ##__VA_ARGS__)

    #if defined(__cpp_lib_source_location)
        [[maybe_unused]] 
        static std::string functionToLocation(const std::string& func)
        {
            std::smatch match;
            if (std::regex_search(func, match, std::regex(R"((?:\b(\w+)\b::)?\b(\w+)\b(?=\s*\())"))) {
                if (match[1].matched) {
                    return match[1].str() + "::" + match[2].str();
                }
                else {
                    return match[2].str();
                }
            }
            return "";
        }

        #define METHOD_NAME functionToLocation(std::source_location::current().function_name())
    #else
        #define METHOD_NAME "unknown"
    #endif
    
    class ILogger
    {
    public:
        static void init(int prio);
        static void shutdown();

        enum class LogLevel 
        {
            Trace = 0,
            Info,
            Warn,
            Error,
            Fatal,
            Highlight,
            Abort = 99
        };

        struct LogData
        {
            LogLevel level;
            Timestamp ts;
            std::string loc;
            std::string msg;
        };

        virtual ~ILogger() = default;
 
        template<typename... T>
        void trace(const bool background, std::string location, std::format_string<T...> fmt, T&&... args)
        {
            background ?
                addMessage(LogLevel::Trace, OSAL::currentTime(), std::move(location), std::format(fmt, std::forward<T>(args)...)) :
                printMessage(LogLevel::Trace, OSAL::currentTime(), location, std::format(fmt, std::forward<T>(args)...));
        }

        template<typename... T>
        void info(const bool background, std::string location, std::format_string<T...> fmt, T&&... args)
        {
            background ?
                addMessage(LogLevel::Info, OSAL::currentTime(), std::move(location), std::format(fmt, std::forward<T>(args)...)) :
                printMessage(LogLevel::Info, OSAL::currentTime(), location, std::format(fmt, std::forward<T>(args)...));
        }

        template<typename... T>
        void warn(const bool background, std::string location, std::format_string<T...> fmt, T&&... args)
        {
            background ?
                addMessage(LogLevel::Warn, OSAL::currentTime(), std::move(location), std::format(fmt, std::forward<T>(args)...)) :
                printMessage(LogLevel::Warn, OSAL::currentTime(), location, std::format(fmt, std::forward<T>(args)...));
        }

        template<typename... T>
        void error(const bool background, std::string location, std::format_string<T...> fmt, T&&... args)
        {
            background ?
                addMessage(LogLevel::Error, OSAL::currentTime(), std::move(location), std::format(fmt, std::forward<T>(args)...)) :
                printMessage(LogLevel::Error, OSAL::currentTime(), location, std::format(fmt, std::forward<T>(args)...));
        }

        template<typename... T>
        void fatal(const bool background, std::string location, std::format_string<T...> fmt, T&&... args)
        {
            background ?
                addMessage(LogLevel::Fatal, OSAL::currentTime(), std::move(location), std::format(fmt, std::forward<T>(args)...)) :
                printMessage(LogLevel::Fatal, OSAL::currentTime(), location, std::format(fmt, std::forward<T>(args)...));
        }

        template<typename... T>
        void highlight(const bool background, std::string location, std::format_string<T...> fmt, T&&... args)
        {
            background ?
                addMessage(LogLevel::Highlight, OSAL::currentTime(), std::move(location), std::format(fmt, std::forward<T>(args)...)) :
                printMessage(LogLevel::Highlight, OSAL::currentTime(), location, std::format(fmt, std::forward<T>(args)...));
        }

    protected:
        virtual void addMessage(const LogLevel level, const Timestamp& timestamp, std::string&& location, std::string&& message) = 0;
        virtual void printMessage(const LogLevel level, const Timestamp& timestamp, const std::string& location, const std::string& message) const = 0;
    };

    extern ILogger* g_logger;
    extern std::span<std::byte> g_loggerStack;

    template<size_t MsgQueueSize, size_t ThreadStackSize>
    class Logger : public ILogger
    {
    public:
        Logger(int prio)
        {
            Utils::setupStaticMessageQueue(m_queue);
            Utils::setupStaticThread(m_thread);
        }

        ~Logger()
        {
            m_running = false;

            LogData* abortMsg = createMessage(LogLevel::Abort, Timestamp{}, "", "");
            m_queue.queue.get()->push(OSAL::MessageQueue::MsgType(std::in_place_type<LogData*>, abortMsg));

            Utils::shutdownStaticThread(m_thread);
        }

    private:
        template<typename ...Args>
        LogData* createMessage(Args&&... args)
        {
            return m_msgPool.template construct<LogData>(std::forward<Args>(args)...);
        }

        void destroyMessage(LogData* ptr)
        {
            m_msgPool.destroy(ptr);
        }

        void addMessage(const LogLevel level, const Timestamp& timestamp, std::string&& location, std::string&& message) override
        {
            if (!m_msgPool.hasSpace())
                return;

            LogData* msg = createMessage(level, timestamp, std::move(location), std::move(message));
            m_queue.queue.get()->push(OSAL::MessageQueue::MsgType(std::in_place_type<LogData*>, msg));
        }

        void printMessage(const LogLevel level, const Timestamp& timestamp, const std::string& location, const std::string& message) const override
        {
            auto consoleColor = ConsoleColor::Standard;
            bool err = false;
            switch (level) {
                case LogLevel::Trace:       consoleColor = ConsoleColor::Standard;  err = false;    break;
                case LogLevel::Info:        consoleColor = ConsoleColor::Green;     err = false;    break;
                case LogLevel::Warn:        consoleColor = ConsoleColor::Yellow;    err = false;    break;
                case LogLevel::Error:       consoleColor = ConsoleColor::Red;       err = true;     break;
                case LogLevel::Fatal:       consoleColor = ConsoleColor::Red;       err = true;     break;
                case LogLevel::Highlight:   consoleColor = ConsoleColor::Cyan;      err = false;    break;
                case LogLevel::Abort:       return;
            }

            const auto msg = std::format("[{}] <{}>: {}", timestamp.timeStr(), location, message);

            OSAL::setConsoleColor(consoleColor);
            if (err) OSAL::eprintln(msg.c_str());
            else OSAL::println(msg.c_str());
            OSAL::setConsoleColor(ConsoleColor::Standard);
        }

        void loggingTask()
        {
            StaticQueue<OSAL::MessageQueue::MsgType, 8> localQueue;

            m_running = true;
            while (m_running || !m_queue.queue.get()->empty()) {
                if (!m_queue.queue.get()->popAvail(localQueue))
                    continue;

                for (auto& anyMsg : localQueue) {
                    LogData* msg = anyMsg.asUnchecked<LogData*>();
                    const auto& [level, timestamp, location, message] = *msg;

                    if (level == LogLevel::Abort)
                        m_running = false;
                    else
                        printMessage(level, timestamp, location, message);

                    destroyMessage(msg);
                }
                localQueue.clear();
            }
        }

        std::atomic_bool m_running;
        Utils::StaticThread<OSAL::StaticImpl::Thread, []() -> void {
            static_cast<Logger<MsgQueueSize, ThreadStackSize>*>(g_logger)->loggingTask(); 
        }, ThreadStackSize, 10> m_thread;
        Utils::StaticMessageQueue<OSAL::StaticImpl::MessageQueue, MsgQueueSize> m_queue;
        StaticBlockPool<MsgQueueSize, allocData<LogData>()> m_msgPool;
    };

#else

    #if !defined(EATK_DISABLE_LOGGING) 
        #warning "Logging not supported"
    #endif

    #define EATK_INIT_LOG()
    #define EATK_SHUTDOWN_LOG()

    #define EATK_TRACE_LOC(loc, fmt, ...)
    #define EATK_INFO_LOC(loc, fmt, ...) 
    #define EATK_WARN_LOC(loc, fmt, ...) 
    #define EATK_ERROR_LOC(loc, fmt, ...)
    #define EATK_FATAL_LOC(loc, fmt, ...)
    #define EATK_HIGHLIGHT_LOC(loc, fmt, ...)

    #define EATK_TRACE(fmt, ...)
    #define EATK_INFO(fmt, ...)	
    #define EATK_WARN(fmt, ...)	
    #define EATK_ERROR(fmt, ...)
    #define EATK_FATAL(fmt, ...)
    #define EATK_HIGHLIGHT(fmt, ...)

    #define EATK_TRACE_NOW(fmt, ...)
    #define EATK_INFO_NOW(fmt, ...)	
    #define EATK_WARN_NOW(fmt, ...)	
    #define EATK_ERROR_NOW(fmt, ...)
    #define EATK_FATAL_NOW(fmt, ...)
    #define EATK_HIGHLIGHT_NOW(fmt, ...)

#endif

