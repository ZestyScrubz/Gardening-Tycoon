#include "Treasury.h"
#include "Exceptions.h"

Treasury::Treasury(double startingFunds) : balance(startingFunds) {}

void Treasury::credit(double amount) {
    if (amount > 0.0) {
        balance += amount;
    }
}

void Treasury::debit(double amount) {
    if (!canAfford(amount)) {
        throw InsolventFundsException("Transaction rejected: Insufficient funds in treasury.");
    }
    balance -= amount;
}

bool Treasury::canAfford(double cost) const {
    return balance >= cost && cost >= 0.0;
}