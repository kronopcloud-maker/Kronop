#include "AI4_Input_Push.hpp"
#include <iostream>

AI4_Input_Push::AI4_Input_Push() {
    // Constructor
}

AI4_Input_Push::~AI4_Input_Push() {
    // Destructor
}

void AI4_Input_Push::storeData(const std::string& data) {
    this->data = data;
    std::cout << "AI4_Input_Push: Stored data: " << data << std::endl;
}

std::string AI4_Input_Push::getData() {
    return this->data;
}
