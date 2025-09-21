#pragma once

#include <userver/storages/postgres/io/chrono.hpp>


namespace call_flow_processor::models {

struct Connection {
    std::int64_t connection_id;
    std::int64_t call_id;
    std::string phone;

    userver::storages::postgres::TimePointTz initiated_at;
    std::optional<userver::storages::postgres::TimePointTz> answered_at;
    std::optional<userver::storages::postgres::TimePointTz> finished_at;
};

}  // namespace call_flow_processor::models