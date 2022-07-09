#ifndef LOG_INTERFACE
#define LOG_INTERFACE

#include <iostream>
#include <string>

#define cstr(str) u8##str

namespace Log
{
    enum LogLevel
    {
        eDebug = 0,
        eInfo,
        eWarning,
        eError,
        eFatal
    };

    const char* LevelToStr(LogLevel level)
    {
        switch (level)
        {
            case eDebug:
                return " - Debug - ";
            case eInfo:
                return " - Info - ";
            case eWarning:
                return " - Warning - ";
            case eError:
                return " - Error - ";
            case eFatal:
                return " - Fatal - ";
        }

        return " - Unknown - ";
    }

    struct ILogger
    {
        virtual void writeLog(const std::string& module, LogLevel level, const std::string& msg) = 0;
    };

    template <typename LoggerType>
    using is_logger = std::enable_if_t<std::is_base_of_v<ILogger, LoggerType> && std::is_default_constructible_v<LoggerType>>;

    template <typename LoggerType, typename = is_logger<LoggerType>>
    class Logger
    {
    private:
        static ILogger* m_logger;

    public:
        static ILogger* GetInstance()
        {
            if (m_logger != nullptr) return m_logger;
            return new LoggerType();
        }
    };

    template <typename LoggerType, typename Tag>
    ILogger* Logger<LoggerType, Tag>::m_logger = nullptr;

    struct EmptyLogger : ILogger
    {
        void writeLog(const std::string& module, LogLevel level, const std::string& msg) override {}
    };

    struct ConsoleLogger : ILogger
    {
        void writeLog(const std::string& module, LogLevel level, const std::string& msg) override
        {
            std::cout << module << LevelToStr(level) << msg << std::endl;
        }
    };
}

#endif
