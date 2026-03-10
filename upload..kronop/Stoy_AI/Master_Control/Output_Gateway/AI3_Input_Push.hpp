#ifndef AI3_INPUT_PUSH_HPP
#define AI3_INPUT_PUSH_HPP

#include <string>

class AI3_Input_Push {
public:
    AI3_Input_Push();
    ~AI3_Input_Push();

    // Store processed data
    void storeData(const std::string& data);

    // Get stored data
    std::string getData();

private:
    std::string data;
};

#endif // AI3_INPUT_PUSH_HPP
