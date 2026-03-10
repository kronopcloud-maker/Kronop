#include "AI5_Input_Push.hpp"
#include <iostream>

AI5_Input_Push::AI5_Input_Push() {
    // Constructor
}

AI5_Input_Push::~AI5_Input_Push() {
    // Destructor
}

void AI5_Input_Push::storeData(const std::string& data) {
    this->data = data;
    std::cout << "AI5_Input_Push: Stored data: " << data << std::endl;
}

std::string AI5_Input_Push::getData() {
    return this->data;
}
