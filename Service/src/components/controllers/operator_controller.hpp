#pragma once

#include <userver/components/loggable_component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <vector>
#include <string>
#include "models/operator.hpp"

namespace call_flow_processor::components::controllers {

class OperatorController final : public userver::components::LoggableComponentBase {
public:
    static constexpr const char* kName;

    OperatorController(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context);

    void Save(std::vector<models::Operator>&& operators);
    std::vector<models::Operator> GetOperators(const std::vector<std::int64_t>& operator_ids);
    std::vector<models::Operator> GetAllOperators();

protected:
    userver::storages::postgres::ClusterPtr pg_;
};

}  // namespace call_flow_processor::components::controllers
