#pragma once

#include <string>
#include <vector>
#include <userver/storages/postgres/io/time_point_tz.hpp>

namespace call_flow_processor::models {

struct CDR {
    std::string call_id;
    userver::storages::postgres::TimePointTz call_start;
    userver::storages::postgres::TimePointTz call_end;
    std::string caller_number;
    std::string callee_number;
    int duration_sec;
    std::string call_result;
    std::vector<std::string> call_events;
};

} // namespace call_flow_processor::models
