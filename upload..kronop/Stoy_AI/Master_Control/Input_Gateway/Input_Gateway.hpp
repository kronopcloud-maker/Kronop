#ifndef INPUT_GATEWAY_HPP
#define INPUT_GATEWAY_HPP

#include "../Output_Gateway/AI1_Input_Push.hpp"
#include "../Output_Gateway/AI2_Input_Push.hpp"
#include "../Output_Gateway/AI3_Input_Push.hpp"
#include "../Hardware_Optimizer/Hardware_Optimizer.hpp"
#include <string>
#include <vector>

class InputGateway {
public:
    InputGateway();
    ~InputGateway();

    void processVideo(const std::string& videoData);

private:
    AI1_Input_Push ai1InputPush;
    AI2_Input_Push ai2InputPush;
    AI3_Input_Push ai3InputPush;
    HardwareOptimizer optimizer;
};

#endif // INPUT_GATEWAY_HPP
