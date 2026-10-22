#pragma once
#include <random>

class IRandomSource {
public:
    virtual ~IRandomSource() = default;
    virtual double getRandomDouble(double min, double max) = 0;
    virtual int getRandomInt(int min, int max) = 0;
};

class SystemRandomSource : public IRandomSource {
private:
    std::mt19937 rng;
public:
    explicit SystemRandomSource(unsigned int seed = std::random_device{}()) : rng(seed) {}
    double getRandomDouble(double min, double max) override {
        std::uniform_real_distribution<double> dist(min, max);
        return dist(rng);
    }
    int getRandomInt(int min, int max) override {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(rng);
    }
};

class DeterministicRandomSource : public IRandomSource {
private:
    double fixedDouble;
    int fixedInt;
public:
    DeterministicRandomSource(double d = 0.5, int i = 1) : fixedDouble(d), fixedInt(i) {}
    double getRandomDouble(double, double) override { return fixedDouble; }
    int getRandomInt(int, int) override { return fixedInt; }
};