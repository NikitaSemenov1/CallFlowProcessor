#pragma once

#include <string>

namespace call_flow_processor::models {

struct Operator {
    std::int64_t operator_id;
    std::string name;
    std::string extension;
    std::string email;
};

}  // call_flow_processor