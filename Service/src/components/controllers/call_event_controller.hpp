#pragma once

#include <userver/components/loggable_component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/database.hpp>
#include <userver/logging/log.hpp>
#include <vector>
#include <string>
#include "models/call_event.hpp"
#include "components/cdr_upload_info.hpp"

namespace call_flow_processor::components::controllers {

class CallEventController final : public userver::components::LoggableComponentBase {
public:
    static constexpr const char* kName;

    CallEventController(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

    void Save(std::vector<models::CallEvent>&& events);

    std::vector<models::CallEvent> GetEvents(const std::vector<std::int64_t>& call_ids);

protected:
    userver::storages::postgres::ClusterPtr pg_;
    components::CDRUploadInfo& cdr_upload_info_;
};

} // namespace call_flow_processor::components::controllers
