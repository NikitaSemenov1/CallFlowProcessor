#include "cdr_uploader.hpp"
#include <userver/logging/log.hpp>
#include <algorithm>

namespace call_flow_processor::components {

const char* CDRUploader::kName = "cdr-uploader";

CDRUploader::CDRUploader(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context)
    : CDRUploaderBase<models::CDR>(config, context),
      cdr_controller_(context.FindComponent<controllers::CDRController>("cdr-controller")),
      call_controller_(context.FindComponent<controllers::CallController>("call-controller")),
      call_event_controller_(context.FindComponent<controllers::CallEventController>("call-event-controller")),
      operator_controller_(context.FindComponent<controllers::OperatorController>("operator-controller")),
      connection_controller_(context.FindComponent<controllers::ConnectionController>("connection-controller"))
{}

std::string CDRUploader::GetId() { return "internal_cdr"; }

std::vector<models::CDR> CDRUploader::Collect(const std::vector<std::int64_t>& call_ids) {
    std::vector<models::CDR> result;
    if (call_ids.empty()) return result;

    std::vector<models::Call> calls = call_controller_.GetCalls(call_ids);
    std::unordered_map<std::int64_t, models::Call> calls_map;
    for (auto&& call : calls) calls_map[call.id] = std::move(call);

    std::unordered_map<std::int64_t, std::vector<models::CallEvent>> events_map;
    {
        auto all_events = call_event_controller_.GetEvents(call_ids);
        for (auto&& ev : all_events) events_map[ev.call_id].push_back(std::move(ev));
    }

    std::unordered_map<std::int64_t, std::vector<models::Connection>> conn_map;
    {
        auto all_connections = connection_controller_.GetConnections(call_ids);
        for (auto&& c : all_connections) conn_map[c.call_id].push_back(std::move(c));
    }

    std::unordered_map<std::int64_t, models::Operator> operator_map;
    {
        std::vector<std::int64_t> user_ids;
        for (const auto& call : calls) user_ids.push_back(call.user_id);
        user_ids.erase(std::unique(user_ids.begin(), user_ids.end()), user_ids.end());
        auto all_operators = operator_controller_.GetOperators(user_ids);
        for (auto&& op : all_operators) operator_map[op.operator_id] = std::move(op);
    }

    for (auto call_id : call_ids) {
        auto call_it = calls_map.find(call_id);
        if (call_it == calls_map.end()) continue;
        const auto& call = call_it->second;

        auto op_it = operator_map.find(call.user_id);
        if (op_it == operator_map.end()) continue;

        const auto& events = events_map[call_id];
        std::vector<std::string> event_types;
        for (const auto& ev : events) event_types.push_back(ev.event_type);

        models::CDR cdr;
        cdr.call_id        = std::to_string(call.id);
        cdr.call_start     = call.started_at;
        cdr.call_end       = call.finished_at;
        cdr.caller_number  = call.caller_number;
        cdr.callee_number  = call.callee_number;
        cdr.duration_sec   = static_cast<int>((call.finished_at - call.started_at).count());
        cdr.call_result    = call.status;
        cdr.call_events    = std::move(event_types);

        result.push_back(std::move(cdr));
    }

    return result;
}

void CDRUploader::Upload(std::vector<models::CDR>&& data) {
    if (data.empty()) return;
    try {
        cdr_controller_.Save(std::move(data));
        for (const auto& cdr : data) {
            upload_info_.MarkUploaded(GetId(), std::stoll(cdr.call_id));
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "CDRUploader upload batch failed: " << ex.what();
    }
}

} // namespace call_flow_processor::components
