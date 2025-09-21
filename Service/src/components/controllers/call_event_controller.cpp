#include "call_event_controller.hpp"
#include <userver/formats/json.hpp>

namespace call_flow_processor::components::controllers {

const char* CallEventController::kName = "call-event-controller";

CallEventController::CallEventController(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
: userver::components::LoggableComponentBase(config, context),
  pg_{context.FindComponent<userver::components::Postgres>("postgres").GetCluster()},
  cdr_upload_info_(context.FindComponent<components::CDRUploadInfo>("cdr-upload-info"))
{}

void CallEventController::Save(std::vector<models::CallEvent>&& events) {
    if (events.empty()) return;
    try {
        auto trx = pg_->Begin(userver::storages::postgres::ClusterHostType::kMaster);
        std::vector<std::int64_t> finished_call_ids;
        for (const auto& event : events) {
            trx.Execute(
                "INSERT INTO call_events (event_id, call_id, event_type, payload) "
                "VALUES ($1, $2, $3, $4) "
                "ON CONFLICT(event_id) DO UPDATE "
                "SET call_id=EXCLUDED.call_id, event_type=EXCLUDED.event_type, payload=EXCLUDED.payload;",
                event.event_id,
                event.call_id,
                event.event_type,
                userver::formats::json::ToString(event.payload)
            );
            if (event.event_type == "hangup") {
                finished_call_ids.push_back(event.call_id);
            }
        }
        trx.Commit();
        if (!finished_call_ids.empty()) {    
            cdr_upload_info_.BatchStoreFinishedCalls(finished_call_ids);
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "CallEventController Save error: " << ex.what();
        throw;
    }
}

std::vector<models::CallEvent> CallEventController::GetEvents(const std::vector<std::int64_t>& call_ids) {
    std::vector<models::CallEvent> result;
    if (call_ids.empty()) return result;
    try {
        auto trx = pg_->Begin(userver::storages::postgres::ClusterHostType::kMaster);
        auto res = trx.Execute(
            "SELECT event_id, call_id, event_type, payload FROM call_events WHERE call_id = ANY($1)",
            call_ids);

        for (const auto& row : res) {
            models::CallEvent ev;
            ev.event_id = row["event_id"].As<std::int64_t>();
            ev.call_id  = row["call_id"].As<std::int64_t>();
            ev.event_type = row["event_type"].As<std::string>();
            ev.payload = userver::formats::json::FromString(row["payload"].As<std::string>());
            result.emplace_back(std::move(ev));
        }
        trx.Commit();
    } catch (const std::exception& ex) {
        LOG_ERROR() << "CallEventController GetEvents error: " << ex.what();
        throw;
    }
    return result;
}

} // namespace call_flow_processor::components::controllers
