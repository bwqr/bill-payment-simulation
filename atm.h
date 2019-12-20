#ifndef BILL_PAYMENT_SIMULATION_ATM_H
#define BILL_PAYMENT_SIMULATION_ATM_H


#include <fstream>

struct atm {
    int id;
    std::ofstream *stream;
    explicit atm(int id, std::ofstream *stream);
    explicit atm() = default;
};


#endif //BILL_PAYMENT_SIMULATION_ATM_H
