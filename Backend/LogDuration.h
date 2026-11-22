#pragma once

#include <string>
#include <chrono>
#include <iostream>
#include <fstream>

using namespace std::chrono;

class LogDuration
{
public:
    explicit LogDuration(const std::string& msg = " ") :
        message(msg + " : "),
        start(steady_clock::now()),
        file(name, std::ios::app)
    {
    }
    ~LogDuration()
    {
        auto finish = steady_clock::now();
        auto dur = finish - start;
        if (file.is_open())
        {
            file << message << duration_cast<milliseconds>(dur).count() << " ms " << std::endl;
        }
    }

private:
    std::string message;
    steady_clock::time_point start;
    std::string name = "log.txt";
    fstream file;
};
