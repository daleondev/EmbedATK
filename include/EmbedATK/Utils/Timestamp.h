#pragma once

struct Timestamp
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t millisecond;

    Timestamp() = default;
    Timestamp(const std::string& timestamp)
    {
        day			= static_cast<uint16_t>(std::stoi(timestamp.substr(0, 2)));
        month		= static_cast<uint8_t>(std::stoi(timestamp.substr(3, 2)));
        year		= static_cast<uint8_t>(std::stoi(timestamp.substr(6, 4)));
        hour		= static_cast<uint8_t>(std::stoi(timestamp.substr(11, 2)));
        minute		= static_cast<uint8_t>(std::stoi(timestamp.substr(14, 2)));
        second		= static_cast<uint8_t>(std::stoi(timestamp.substr(17, 2)));
        millisecond	= static_cast<uint16_t>(std::stoi(timestamp.substr(20, 3)));
    }

    std::string dateStr() const
    {
        std::stringstream ss;

        day < 10 ? ss << '0' << (uint16_t)day : ss << (uint16_t)day;
        ss << '.';
        month < 10 ? ss << '0' << (uint16_t)month : ss << (uint16_t)month;
        ss << '.';
        ss << year;

        return ss.str();
    }

    std::string timeStr() const
    {
        std::stringstream ss;

        hour < 10 ? ss << '0' << (uint16_t)hour : ss << (uint16_t)hour;
        ss << ':';
        minute < 10 ? ss << '0' << (uint16_t)minute : ss << (uint16_t)minute;
        ss << ':';
        second < 10 ? ss << '0' << (uint16_t)second : ss << (uint16_t)second;
        ss << ':';
        millisecond < 100 ? millisecond < 10 ? ss << "00" << millisecond : ss << '0' << millisecond : ss << millisecond;

        return ss.str();
    }

    std::string dateTimeStr() const
    {
        std::stringstream ss;

        ss << dateStr();
        ss << ' ';
        ss << timeStr();

        return ss.str();
    }
};