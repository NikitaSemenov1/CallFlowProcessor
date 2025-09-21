#pragma once

#include <userver/components/loggable_component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <optional>
#include <vector>
#include <string>
#include "models/connection.hpp"

namespace call_flow_processor::components::controllers {

class ConnectionController final : public userver::components::LoggableComponentBase {
public:
    static constexpr const char* kName;

    ConnectionController(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

    void Save(std::vector<models::Connection>&& connections);
    std::vector<models::Connection> GetConnections(const std::vector<std::int64_t>& connection_ids);

protected:
    userver::storages::postgres::ClusterPtr pg_;
};

} // namespace call_flow_processor::components::controllers
