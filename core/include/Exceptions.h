#pragma once
#include <stdexcept>
#include <string>

class UntilledPlotException : public std::runtime_error {
public:
    explicit UntilledPlotException(const std::string& msg) : std::runtime_error(msg) {}
};

class InsolventFundsException : public std::runtime_error {
public:
    explicit InsolventFundsException(const std::string& msg) : std::runtime_error(msg) {}
};

class IncompatibleToolException : public std::runtime_error {
public:
    explicit IncompatibleToolException(const std::string& msg) : std::runtime_error(msg) {}
};

class InvalidHarvestStateException : public std::runtime_error {
public:
    explicit InvalidHarvestStateException(const std::string& msg) : std::runtime_error(msg) {}
};

class CorruptSaveFileException : public std::runtime_error {
public:
    explicit CorruptSaveFileException(const std::string& msg) : std::runtime_error(msg) {}
};