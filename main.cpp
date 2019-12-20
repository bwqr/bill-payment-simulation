#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include "customer.h"
#include "defs.h"
#include "atm.h"

using namespace std;

//Global var definitions
pthread_mutex_t bill_mutex[NUM_BILLS];
pthread_mutex_t atm_mutex[NUM_ATM];
pthread_mutex_t atm_serve_mutex[NUM_ATM];
pthread_mutex_t atm_customer_mutex[NUM_ATM];
customer *atm_service[NUM_ATM];
int bill_types[NUM_BILLS] = {0};
pthread_t atm_threads[NUM_ATM];
pthread_t *customer_threads;
int num_cust = 0;

void *customer_handler(void *_c);

void *atm_handler(void *_a);

int read_file(char *file, int &num_customers, customer *&customers);

int main(int argc, char **argv) {

    if (argc < 2) {
        cerr << "Error, input file is not given" << endl;
        return -1;
    }

    ofstream stream(string(argv[1]) + "_log.txt");
    customer *customers = nullptr;
    atm *atm[NUM_ATM];

    if (read_file(argv[1], num_cust, customers)) {
        cerr << "Error, input file could not be open" << endl;
        return -1;
    }

    if (!stream.is_open()) {
        cerr << "Error, output file could not be open" << endl;
        goto cleanup;
    }

    for (int i = 0; i < NUM_ATM; ++i) {
        pthread_mutex_lock(&atm_serve_mutex[i]);
        pthread_mutex_lock(&atm_customer_mutex[i]);
        atm[i] = new struct atm(i, &stream);
        pthread_create(&atm_threads[i], nullptr, atm_handler, (void *) atm[i]);
    }

    customer_threads = new pthread_t[num_cust];

    for (int i = 0; i < num_cust; ++i) {
        pthread_create(&customer_threads[i], nullptr, customer_handler, (void *) &customers[i]);
    }

    for (int i = 0; i < num_cust; ++i) {
        pthread_join(customer_threads[i], nullptr);
    }

    //After every customer is served, we can cancel atm threads.
    for (int i = 0; i < NUM_ATM; ++i) {
        pthread_cancel(atm_threads[i]);
        pthread_mutex_unlock(&atm_serve_mutex[i]);
    }

    //Write the output

    cleanup:

    delete[] customers;
    delete[] customer_threads;

    return 0;
}

void *customer_handler(void *_c) {
    auto *cst = (customer *) _c;
    int retval = 0;

    this_thread::sleep_for(chrono::milliseconds(cst->wait));

    pthread_mutex_lock(&atm_mutex[cst->atm]);

    atm_service[cst->atm] = cst;

    pthread_mutex_unlock(&atm_serve_mutex[cst->atm]);
    pthread_mutex_lock(&atm_customer_mutex[cst->atm]);
    pthread_mutex_unlock(&atm_mutex[cst->atm]);

    pthread_exit(&retval);
}

void *atm_handler(void *_a) {
    auto *atm = (struct atm *) _a;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (true) {
        pthread_mutex_lock(&atm_serve_mutex[atm->id]);
        pthread_testcancel();

        customer *cst = atm_service[atm->id];
        pthread_mutex_lock(&bill_mutex[cst->bill_type]);
        cout << cst->payment;
        pthread_mutex_unlock(&atm_customer_mutex[cst->bill_type]);
        pthread_mutex_unlock(&bill_mutex[cst->bill_type]);
    }
#pragma clang diagnostic pop

}

int read_file(char *file, int &num_customers, customer *&customers) {
    ifstream stream(file);

    if (!stream.is_open()) {
        return -1;
    }

    string line, bill_type, word;
    getline(stream, word);
    num_customers = stoi(word);

    customers = new customer[num_customers];


    for (int i = 0; i < num_customers; ++i) {
        customers[i].id = i;
        getline(stream, line);

        istringstream ss(line);

        getline(ss, word, ',');
        customers[i].wait = stoi(word);

        getline(ss, word, ',');
        customers[i].atm = stoi(word) - 1;

        getline(ss, bill_type, ',');

        getline(ss, word, ',');
        customers[i].payment = stoi(word);

        if (bill_type == "cableTV") {
            customers[i].bill_type = CABLE_TV;
        } else if (bill_type == "electricity") {
            customers[i].bill_type = ELECTRICITY;
        } else if (bill_type == "telecommunication") {
            customers[i].bill_type = TELECOMM;
        } else if (bill_type == "gas") {
            customers[i].bill_type = GAS;
        } else if (bill_type == "water") {
            customers[i].bill_type = WATER;
        }
    }

    return 0;
}