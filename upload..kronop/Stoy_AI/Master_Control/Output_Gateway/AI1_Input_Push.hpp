#ifndef AI1_INPUT_PUSH_HPP
#define AI1_INPUT_PUSH_HPP

#include <string>

class AI1_Input_Push {
public:
    AI1_Input_Push();
    ~AI1_Input_Push();

    // Store processed data
    void storeData(const std::string& data);

    // Get stored data
    std::string getData();

private:
    std::string data;
};

#endif // AI1_INPUT_PUSH_HPP
