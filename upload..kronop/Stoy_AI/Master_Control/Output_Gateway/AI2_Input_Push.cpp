#include "AI2_Input_Push.hpp"
#include <iostream>

AI2_Input_Push::AI2_Input_Push() {
    // Constructor
}

AI2_Input_Push::~AI2_Input_Push() {
    // Destructor
}

void AI2_Input_Push::storeData(const std::string& data) {
    this->data = data;
    std::cout << "AI2_Input_Push: Stored data: " << data << std::endl;
}

std::string AI2_Input_Push::getData() {
    return this->data;
}
