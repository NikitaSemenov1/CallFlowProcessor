#pragma once

#include <userver/storages/postgres/io/chrono.hpp>


namespace call_flow_processor::models {


struct Call {
    std::int64_t call_id;                         
    std::string call_type;                        
    std::string scenario_id;                      
    std::string initiator_phone;                  
    std::string media_record_id;                  
    userver::storages::postgres::TimePointTz initiated_at;
    std::optional<userver::storages::postgres::TimePointTz> finished_at;
    userver::formats::json::Value context;
};

}  // namespace call_flow_processor::models