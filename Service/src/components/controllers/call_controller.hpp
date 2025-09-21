#pragma once

#include <userver/components/loggable_component_base.hpp>
#include <userver/storages/postgres/database.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <vector>
#include <string>
#include "models/call.hpp"

namespace call_flow_processor::components::controllers {

class CallController final : public userver::components::LoggableComponentBase {
public:
    static constexpr const char* kName;

    CallController(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

    void Save(std::vector<models::Call> &&calls);
    std::vector<models::Call> GetCalls(const std::vector<std::int64_t> &call_ids);
    models::Call GetCall(std::int64_t call_id);

protected:
    const userver::storages::postgres::ClusterPtr pg_;
};

}  // namespace call_flow_processor::components::controllers
