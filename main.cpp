#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <atomic>
#include "customer.h"
#include "defs.h"
#include "atm.h"

using namespace std;

//Global var definitions
atomic_int bill_atomic[NUM_BILLS];
pthread_mutex_t atm_mutex[NUM_ATM];
pthread_mutex_t atm_serving_mutex[NUM_ATM];
pthread_mutex_t atm_served_mutex[NUM_ATM];
pthread_mutex_t stream_mutex;
customer *atm_service[NUM_ATM];
pthread_t atm_threads[NUM_ATM];
pthread_t *customer_threads;
int num_cust = 0;

string bill_types[] = {"cableTV", "electricity", "gas", "telecommunication", "water"};

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

    //Initially set all bills payments to zero
    for (auto & i : bill_atomic) {
        i.store(0);
    }

    //Create the ATM
    for (int i = 0; i < NUM_ATM; ++i) {
        pthread_mutex_lock(&atm_serving_mutex[i]);
        pthread_mutex_lock(&atm_served_mutex[i]);
        atm[i] = new struct atm(i, &stream);
        pthread_create(&atm_threads[i], nullptr, atm_handler, (void *) atm[i]);
    }

    customer_threads = new pthread_t[num_cust];

    //Create customers
    for (int i = 0; i < num_cust; ++i) {
        pthread_create(&customer_threads[i], nullptr, customer_handler, (void *) &customers[i]);
    }

    //Wait customers
    for (int i = 0; i < num_cust; ++i) {
        pthread_join(customer_threads[i], nullptr);
    }

    //After every customer is served, we can cancel atm threads.
    for (int i = 0; i < NUM_ATM; ++i) {
        pthread_cancel(atm_threads[i]);
        pthread_mutex_unlock(&atm_serving_mutex[i]);
        pthread_join(atm_threads[i], nullptr);
    }

    //Write the output
    stream << "All payments are completed." << endl;
    stream << bill_types[CABLE_TV] << ": " << to_string(bill_atomic[CABLE_TV]) << "TL" << endl;
    stream << bill_types[ELECTRICITY] << ": " << to_string(bill_atomic[ELECTRICITY]) << "TL" << endl;
    stream << bill_types[GAS] << ": " << to_string(bill_atomic[GAS]) << "TL" << endl;
    stream << bill_types[TELECOMM] << ": " << to_string(bill_atomic[TELECOMM]) << "TL" << endl;
    stream << bill_types[WATER] << ": " << to_string(bill_atomic[WATER]) << "TL" << endl;

    cleanup:
    delete[] customers;
    delete[] customer_threads;
    for (auto & i : atm) {
        delete i;
    }

    return 0;
}

void *customer_handler(void *_c) {
    auto *cst = (customer *) _c;
    int retval = 0;
    //Wait our time
    this_thread::sleep_for(chrono::milliseconds(cst->wait));
    //Reserve the atm.
    pthread_mutex_lock(&atm_mutex[cst->atm]);
    // Give our service description to atm
    atm_service[cst->atm] = cst;
    //Let the atm serve us.
    pthread_mutex_unlock(&atm_serving_mutex[cst->atm]);
    //Check if atm is finished our service.
    pthread_mutex_lock(&atm_served_mutex[cst->atm]);
    //Let other customers use this atm
    pthread_mutex_unlock(&atm_mutex[cst->atm]);

    pthread_exit(&retval);
}

void *atm_handler(void *_a) {
    auto *atm = (struct atm *) _a;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (true) {
        //Check if any customer is pending
        pthread_mutex_lock(&atm_serving_mutex[atm->id]);
        //Check if we should terminate our thread.
        pthread_testcancel();
        //Get customer service description
        customer *cst = atm_service[atm->id];
        // Pay the bills
        bill_atomic[cst->bill_type].fetch_add(cst->payment);
        //Log the payment
        string s =
                "Customer" + to_string(cst->id) + ',' + to_string(cst->payment) + "TL," + bill_types[cst->bill_type] +
                '\n';
        //Lock the output stream
        pthread_mutex_lock(&stream_mutex);
        *atm->stream << s;
        //Unlock the output stream
        pthread_mutex_unlock(&stream_mutex);
        //Inform the customer about service completion.
        pthread_mutex_unlock(&atm_served_mutex[atm->id]);
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
        customers[i].id = i + 1;
        getline(stream, line);

        istringstream ss(line);

        getline(ss, word, ',');
        customers[i].wait = stoi(word);

        getline(ss, word, ',');
        customers[i].atm = stoi(word) - 1;

        getline(ss, bill_type, ',');

        getline(ss, word, ',');
        customers[i].payment = stoi(word);

        if (bill_type == bill_types[CABLE_TV]) {
            customers[i].bill_type = CABLE_TV;
        } else if (bill_type == bill_types[ELECTRICITY]) {
            customers[i].bill_type = ELECTRICITY;
        } else if (bill_type == bill_types[TELECOMM]) {
            customers[i].bill_type = TELECOMM;
        } else if (bill_type == bill_types[GAS]) {
            customers[i].bill_type = GAS;
        } else {
            customers[i].bill_type = WATER;
        }
    }

    return 0;
}