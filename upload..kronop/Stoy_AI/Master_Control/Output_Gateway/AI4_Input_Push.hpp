#ifndef AI4_INPUT_PUSH_HPP
#define AI4_INPUT_PUSH_HPP

#include <string>

class AI4_Input_Push {
public:
    AI4_Input_Push();
    ~AI4_Input_Push();

    // Store processed data
    void storeData(const std::string& data);

    // Get stored data
    std::string getData();

private:
    std::string data;
};

#endif // AI4_INPUT_PUSH_HPP
