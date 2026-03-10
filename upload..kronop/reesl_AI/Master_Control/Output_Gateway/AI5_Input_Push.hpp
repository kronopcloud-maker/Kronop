#ifndef AI5_INPUT_PUSH_HPP
#define AI5_INPUT_PUSH_HPP

#include <string>

class AI5_Input_Push {
public:
    AI5_Input_Push();
    ~AI5_Input_Push();

    // Store processed data
    void storeData(const std::string& data);

    // Get stored data
    std::string getData();

private:
    std::string data;
};

#endif // AI5_INPUT_PUSH_HPP
