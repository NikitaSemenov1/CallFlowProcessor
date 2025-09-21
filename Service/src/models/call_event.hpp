#pragma once

#include <userver/formats/json.hpp>

namespace call_flow_processor::models {


struct CallEvent {
    std::int64_t event_id;                
    std::int64_t call_id;                 
    std::string event_type;               
    userver::formats::json::Value payload;
};


}  // namespace call_flow_processor::models