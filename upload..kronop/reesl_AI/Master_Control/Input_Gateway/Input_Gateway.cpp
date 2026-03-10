#include "Input_Gateway.hpp"
#include <iostream>

InputGateway::InputGateway() {
    // Constructor
}

InputGateway::~InputGateway() {
    // Destructor
}

void InputGateway::processVideo(const std::string& videoData) {
    AIMode mode = optimizer.selectAI();

    std::cout << "InputGateway: Selected AI mode: " << (mode == ONE_AI ? "ONE_AI" : mode == TWO_AI ? "TWO_AI" : "THREE_AI") << ", sending whole video to enabled AIs." << std::endl;

    // Store whole video in enabled senders
    if (mode == ONE_AI || mode == TWO_AI || mode == THREE_AI) {
        ai1InputPush.storeData(videoData);
    }
    if (mode == TWO_AI || mode == THREE_AI) {
        ai2InputPush.storeData(videoData);
    }
    if (mode == THREE_AI) {
        ai3InputPush.storeData(videoData);
    }

    std::cout << "InputGateway: Whole video stored in senders." << std::endl;
}
