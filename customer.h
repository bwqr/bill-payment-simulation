#ifndef BILL_PAYMENT_SIMULATION_CUSTOMER_H
#define BILL_PAYMENT_SIMULATION_CUSTOMER_H

struct customer {
    int id;
    int wait;
    int atm;
    int bill_type;
    int payment;
    explicit customer(int id, int wait, int atm, int bill_type, int payment);
    explicit customer() = default;
};

#endif //BILL_PAYMENT_SIMULATION_CUSTOMER_H
