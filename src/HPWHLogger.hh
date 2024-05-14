#ifndef HPWHLOGGER_hh
#define HPWHLOGGER_hh

#include <courier/courier.h>

#include "courier/helpers.h"

class Logger : public Courier::Courier
{
  public:
    Logger() = default;

    enum class MessageLevel
    {
        all,
        debug,
        info,
        warning,
        error
    };
    MessageLevel message_level {MessageLevel::error};

  protected:
    void receive_error(const std::string& message) override
    {
        write_message("ERROR", message);
        throw std::runtime_error(message);
    }
    void receive_warning(const std::string& message) override
    {
        if (message_level <= MessageLevel::warning)
        {
            write_message("WARNING", message);
        }
    }
    void receive_info(const std::string& message) override
    {
        if (message_level <= MessageLevel::info)
        {
            write_message("INFO", message);
        }
    }
    void receive_debug(const std::string& message) override
    {
        if (message_level <= MessageLevel::debug)
        {
            write_message("DEBUG", message);
        }
    }
    void write_message(const std::string& message_type, const std::string& message)
    {
        std::cout << fmt::format("[{}]{} {}", message_type, "", message) << std::endl;
    }
};

class Dispatcher : public Courier::Sender
{
  public:
    Dispatcher() = default;
    explicit Dispatcher(
        std::string name_in,
        const std::shared_ptr<Courier::Courier>& courier_in = std::make_shared<Logger>())
        : Sender(name_in, courier_in)
    {
    }

    void set_courier(std::shared_ptr<Courier::Courier> courier_in)
    {
        courier = std::move(courier_in);
    }
    std::shared_ptr<Courier::Courier> get_courier() { return courier; };
};

#endif
