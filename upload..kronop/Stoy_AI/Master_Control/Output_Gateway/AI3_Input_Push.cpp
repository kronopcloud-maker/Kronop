#include "AI3_Input_Push.hpp"
#include <iostream>

AI3_Input_Push::AI3_Input_Push() {
    // Constructor
}

AI3_Input_Push::~AI3_Input_Push() {
    // Destructor
}

void AI3_Input_Push::storeData(const std::string& data) {
    this->data = data;
    std::cout << "AI3_Input_Push: Stored data: " << data << std::endl;
}

std::string AI3_Input_Push::getData() {
    return this->data;
}
