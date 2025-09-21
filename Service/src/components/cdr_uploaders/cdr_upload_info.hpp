#pragma once

#include <userver/components/loggable_component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <vector>
#include <string>

namespace call_flow_processor::components {

class CDRUploadInfo final : public userver::components::LoggableComponentBase {
public:
    static constexpr const char* kName;

    CDRUploadInfo(const userver::components::ComponentConfig& config,
                  const userver::components::ComponentContext& context);

    std::vector<std::int64_t> GetFinishedCallIds() const;

    void BatchStoreFinishedCalls(const std::vector<std::int64_t>& call_ids);

    void UpsertPending(const std::string& cdr_type, std::int64_t call_id);

    std::vector<std::int64_t> GetPendingCallIds(const std::string& cdr_type, std::size_t limit = 1000);

    void MarkUploaded(const std::string& cdr_type, std::int64_t call_id);

protected:
    userver::storages::postgres::ClusterPtr pg_;
};

} // namespace call_flow_processor::components
