#include "cdr_controller.hpp"

namespace call_flow_processor::components::controllers {

const char* CDRController::kName = "cdr-controller";

CDRController::CDRController(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
: userver::components::LoggableComponentBase(config, context),
  pg_{context.FindComponent<userver::components::Postgres>("postgres").GetCluster()}
{}

void CDRController::Save(std::vector<models::CDR>&& cdrs) {
    if (cdrs.empty()) return;
    try {
        auto trx = pg_->Begin(userver::storages::postgres::ClusterHostType::kMaster);
        for (const auto& cdr : cdrs) {
            trx.Execute(
                "INSERT INTO cdrs "
                "(call_id, call_start, call_end, caller_number, callee_number, duration_sec, call_result, call_events) "
                "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) "
                "ON CONFLICT (call_id) DO UPDATE SET "
                "call_start=EXCLUDED.call_start, "
                "call_end=EXCLUDED.call_end, "
                "caller_number=EXCLUDED.caller_number, "
                "callee_number=EXCLUDED.callee_number, "
                "duration_sec=EXCLUDED.duration_sec, "
                "call_result=EXCLUDED.call_result, "
                "call_events=EXCLUDED.call_events;",
                cdr.call_id,
                cdr.call_start,
                cdr.call_end,
                cdr.caller_number,
                cdr.callee_number,
                cdr.duration_sec,
                cdr.call_result,
                cdr.call_events
            );
        }
        trx.Commit();
    } catch (const std::exception& ex) {
        LOG_ERROR() << "CDRController Save error: " << ex.what();
        throw;
    }
}

std::vector<models::CDR> CDRController::GetCDRs(const std::vector<std::string>& call_ids) const {
    std::vector<models::CDR> result;
    if (call_ids.empty()) return result;
    try {
        auto trx = pg_->Begin(userver::storages::postgres::ClusterHostType::kSlave);
        auto res = trx.Execute(
            "SELECT call_id, call_start, call_end, caller_number, callee_number, "
            "duration_sec, call_result, call_events FROM cdrs WHERE call_id = ANY($1)",
            call_ids
        );
        for (const auto& row : res) {
            models::CDR cdr;
            cdr.call_id = row["call_id"].As<std::string>();
            cdr.call_start = row["call_start"].As<userver::storages::postgres::TimePointTz>();
            cdr.call_end = row["call_end"].As<userver::storages::postgres::TimePointTz>();
            cdr.caller_number = row["caller_number"].As<std::string>();
            cdr.callee_number = row["callee_number"].As<std::string>();
            cdr.duration_sec = row["duration_sec"].As<int>();
            cdr.call_result = row["call_result"].As<std::string>();
            cdr.call_events = row["call_events"].As<std::vector<std::string>>();
            result.emplace_back(std::move(cdr));
        }
        trx.Commit();
    } catch (const std::exception& ex) {
        LOG_ERROR() << "CDRController GetCDRs error: " << ex.what();
        throw;
    }
    return result;
}

} // namespace call_flow_processor::components::controllers
