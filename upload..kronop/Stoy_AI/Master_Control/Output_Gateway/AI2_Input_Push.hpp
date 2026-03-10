#ifndef AI2_INPUT_PUSH_HPP
#define AI2_INPUT_PUSH_HPP

#include <string>

class AI2_Input_Push {
public:
    AI2_Input_Push();
    ~AI2_Input_Push();

    // Store processed data
    void storeData(const std::string& data);

    // Get stored data
    std::string getData();

private:
    std::string data;
};

#endif // AI2_INPUT_PUSH_HPP
