#pragma once

#include "EmbedATK/OSAL/OSAL.h"

#include "EmbedATK/Memory/Container.h"

#include <format>
#include <source_location>

#if !defined(EATK_DISABLE_LOGGING) && defined(__cpp_lib_format)

    #include <regex>

    #define EATK_INIT_LOG()                     ILogger::init()
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
        static void init();
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

    template<size_t MSG_QUEUE_SIZE>
    class Logger : public ILogger
    {
    public:
        Logger()
        {
            OSAL::createThread(m_thread, g_loggerStack, &Logger::loggingTask, this);
            m_thread.get()->setPriority(10);
            
            OSAL::createMessageQueue<LogData*, MSG_QUEUE_SIZE>(m_queue);

            m_thread.get()->start();
        }

        ~Logger()
        {
            m_running = false;

            auto abortMsg = make_static_unique<LogData>(
                &m_msgPool,
                LogLevel::Abort, 
                Timestamp{}, 
                "", 
                ""
            );
            m_queue.get()->push(abortMsg.release());

            m_thread.get()->shutdown();
        }

    private:
        template<typename ...Args>
        LogData* createMessage(Args&&... args)
        {
            static constexpr auto alloc = allocData<LogData>();
            auto* ptr = static_cast<LogData*>(m_msgPool.allocate(alloc.size, alloc.align));
            std::construct_at<LogData>(ptr, std::forward<Args>(args)...);
            return ptr;
        }

        void destroyMessage(LogData* ptr)
        {
            static constexpr auto alloc = allocData<LogData>();
            std::destroy_at(ptr);
            m_msgPool.deallocate(ptr, alloc.size, alloc.align);
        }

        void addMessage(const LogLevel level, const Timestamp& timestamp, std::string&& location, std::string&& message) override
        {
            auto* msg = createMessage(level, timestamp, std::move(location), std::move(message));
            m_queue.get()->push(msg);
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

            OSAL::setConsoleColor(consoleColor);
            if (err)
                OSAL::eprintln(std::format("[{}] <{}>: {}", timestamp.timeStr(), location, message).c_str());
            else
                OSAL::println(std::format("[{}] <{}>: {}", timestamp.timeStr(), location, message).c_str());
            OSAL::setConsoleColor(ConsoleColor::Standard);
        }

        void loggingTask()
        {
            StaticQueue<LogData*, 64> localQueue;

            m_running = true;
            while (m_running || !m_queue.get()->empty()) {
                if (!m_queue.get()->popAvail(localQueue))
                    continue;

                for (LogData* msg : localQueue) {
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
        OSAL_THREAD m_thread;
        OSAL_MESSAGE_QUEUE(LogData*, MSG_QUEUE_SIZE) m_queue;

        StaticBlockPool<MSG_QUEUE_SIZE, allocData<LogData>()> m_msgPool;
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

