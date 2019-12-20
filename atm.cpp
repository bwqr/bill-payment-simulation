#include "atm.h"

atm::atm(int id, std::ofstream *stream) {
    this->id = id;
    this->stream = stream;
}
