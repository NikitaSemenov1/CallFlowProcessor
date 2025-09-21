#pragma once

#include <userver/components/loggable_component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/logging/log.hpp>
#include <vector>
#include <string>
#include "models/cdr.hpp"

namespace call_flow_processor::components::controllers {

class CDRController final : public userver::components::LoggableComponentBase {
public:
    static constexpr const char* kName;

    CDRController(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

    void Save(std::vector<models::CDR>&& cdrs);
    std::vector<models::CDR> GetCDRs(const std::vector<std::string>& call_ids) const;

protected:
    userver::storages::postgres::ClusterPtr pg_;
};

} // namespace call_flow_processor::components::controllers
