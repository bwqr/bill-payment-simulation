cmake_minimum_required(VERSION 3.15)
project(bill_payment_simulation)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpthread")

add_executable(bill_payment_simulation main.cpp defs.h customer.cpp customer.h atm.cpp atm.h)