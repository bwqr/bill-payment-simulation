#include "customer.h"

customer::customer(const int id, const int wait, const int atm, const int bill_type, const int payment) {
    this->id = id;
    this->wait = wait;
    this->atm = atm;
    this->bill_type = bill_type;
    this->payment = payment;
}
