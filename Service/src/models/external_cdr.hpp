#pragma once

#include <string>
#include <optional>
#include <userver/storages/postgres/io/time_point_tz.hpp>

namespace call_flow_processor::models {

struct ExternalCDR {
    std::string call_id;
    userver::storages::postgres::TimePointTz call_start;
    userver::storages::postgres::TimePointTz call_end;
    std::string caller_number;
    std::optional<std::string> operator_id;
    std::optional<std::string> operator_name;
    std::string agent_status;
    int wait_sec;
    int talk_sec;
    std::string end_reason;
};

} // namespace call_flow_processor::models
