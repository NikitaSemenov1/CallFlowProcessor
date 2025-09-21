#include "external_cdr_uploader.hpp"
#include <userver/logging/log.hpp>
#include <algorithm>
#include <optional>

namespace call_flow_processor::components {

const char* ExternalCDRUploader::kName = "external-cdr-uploader";

ExternalCDRUploader::ExternalCDRUploader(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : CDRUploaderBase<models::ExternalCDR>(config, context),
      call_controller_(context.FindComponent<controllers::CallController>("call-controller")),
      call_event_controller_(context.FindComponent<controllers::CallEventController>("call-event-controller")),
      operator_controller_(context.FindComponent<controllers::OperatorController>("operator-controller")),
      connection_controller_(context.FindComponent<controllers::ConnectionController>("connection-controller")),
      http_client_(context.FindComponent<userver::clients::http::Client>("http-client")),
      upload_url_(config["upload-url"].As<std::string>())
{}

std::string ExternalCDRUploader::GetId() { return "external_cdr"; }

std::vector<models::ExternalCDR> ExternalCDRUploader::Collect(const std::vector<std::int64_t>& call_ids) {
    std::vector<models::ExternalCDR> result;
    if (call_ids.empty()) return result;

    std::vector<models::Call> calls = call_controller_.GetCalls(call_ids);
    std::unordered_map<std::int64_t, models::Call> calls_map;
    for (auto&& call : calls) calls_map[call.id] = std::move(call);

    auto all_events = call_event_controller_.GetEvents(call_ids);
    std::unordered_map<std::int64_t, std::vector<models::CallEvent>> events_map;
    for (auto&& ev : all_events) events_map[ev.call_id].push_back(std::move(ev));

    auto all_connections = connection_controller_.GetConnections(call_ids);
    std::unordered_map<std::int64_t, std::vector<models::Connection>> conn_map;
    for (auto&& c : all_connections) conn_map[c.call_id].push_back(std::move(c));

    std::vector<std::int64_t> user_ids;
    for (const auto& call : calls) user_ids.push_back(call.user_id);
    user_ids.erase(std::unique(user_ids.begin(), user_ids.end()), user_ids.end());
    auto all_operators = operator_controller_.GetOperators(user_ids);
    std::unordered_map<std::int64_t, models::Operator> operator_map;
    for (auto&& op : all_operators) operator_map[op.operator_id] = std::move(op);

    for (auto call_id : call_ids) {
        auto call_it = calls_map.find(call_id);
        if (call_it == calls_map.end()) continue;
        const auto& call = call_it->second;

        std::optional<std::string> operator_id;
        std::optional<std::string> operator_name;
        auto op_it = operator_map.find(call.user_id);
        if (op_it != operator_map.end()) {
            operator_id = std::to_string(op_it->second.operator_id);
            operator_name = op_it->second.name;
        }

        std::string agent_status = "NO_ANSWER";
        int wait_sec = 0;
        int talk_sec = 0;
        std::string end_reason = "";

        const auto& events = events_map[call_id];
        for (const auto& ev : events) {
            if (ev.event_type == "answered") agent_status = "ANSWERED";
            if (ev.event_type == "hangup") end_reason = "COMPLETED";
        }

        const auto& conns = conn_map[call_id];
        if (!conns.empty()) {
            auto conn = conns.front();
            if (conn.answered_at && conn.initiated_at) {
                wait_sec = static_cast<int>((conn.answered_at.value() - conn.initiated_at).count());
            }
            if (conn.finished_at && conn.answered_at) {
                talk_sec = static_cast<int>((conn.finished_at.value() - conn.answered_at.value()).count());
            }
        }

        models::ExternalCDR cdr;
        cdr.call_id        = std::to_string(call.id);
        cdr.call_start     = call.started_at;
        cdr.call_end       = call.finished_at;
        cdr.caller_number  = call.caller_number;
        cdr.operator_id    = operator_id;
        cdr.operator_name  = operator_name;
        cdr.agent_status   = agent_status;
        cdr.wait_sec       = wait_sec;
        cdr.talk_sec       = talk_sec;
        cdr.end_reason     = end_reason.empty() ? call.status : end_reason;

        result.push_back(std::move(cdr));
    }

    return result;
}

void ExternalCDRUploader::Upload(std::vector<models::ExternalCDR>&& data) {
    if (data.empty()) return;
    userver::formats::json::ValueBuilder arr;
    for (const auto& cdr : data) {
        userver::formats::json::ValueBuilder ob;
        ob["call_id"] = cdr.call_id;
        ob["call_start"] = userver::formats::json::ValueBuilder(cdr.call_start.TimePoint().time_since_epoch().count());
        ob["call_end"] = userver::formats::json::ValueBuilder(cdr.call_end.TimePoint().time_since_epoch().count());
        ob["caller_number"] = cdr.caller_number;
        if (cdr.operator_id) ob["operator_id"] = cdr.operator_id.value();
        if (cdr.operator_name) ob["operator_name"] = cdr.operator_name.value();
        ob["agent_status"] = cdr.agent_status;
        ob["wait_sec"] = cdr.wait_sec;
        ob["talk_sec"] = cdr.talk_sec;
        ob["end_reason"] = cdr.end_reason;
        arr.PushBack(ob.ExtractValue());
    }
    try {
        auto response = http_client_.CreateRequest()
            .post(upload_url_)
            .timeout(std::chrono::seconds(10))
            .header("Content-Type", "application/json")
            .body(userver::formats::json::ToString(arr.ExtractValue()))
            .perform();

        if (response->status_code == userver::clients::http::HttpStatus::kOk ||
            response->status_code == userver::clients::http::HttpStatus::kCreated) {
            for (const auto& cdr : data) {
                upload_info_.MarkUploaded(GetId(), std::stoll(cdr.call_id));
            }
        } else {
            LOG_ERROR() << "ExternalCDRUploader POST failed: HTTP " << response->status_code;
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "ExternalCDRUploader upload batch exception: " << ex.what();
    }
}

} // namespace call_flow_processor::components
