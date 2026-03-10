#include "AI1_Input_Push.hpp"
#include <iostream>

AI1_Input_Push::AI1_Input_Push() {
    // Constructor
}

AI1_Input_Push::~AI1_Input_Push() {
    // Destructor
}

void AI1_Input_Push::storeData(const std::string& data) {
    this->data = data;
    std::cout << "AI1_Input_Push: Stored data: " << data << std::endl;
}

std::string AI1_Input_Push::getData() {
    return this->data;
}
