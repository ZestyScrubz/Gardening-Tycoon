#pragma once

class Treasury {
private:
    double balance;

public:
    explicit Treasury(double startingFunds = 250.0);
    void credit(double amount);
    void debit(double amount);
    bool canAfford(double cost) const;
    double getBalance() const { return balance; }
    void setBalance(double newBalance) { balance = newBalance; }
};