#include "call_controller.hpp"
#include <userver/logging/log.hpp>

namespace call_flow_processor::components::controllers {

const char* CallController::kName = "call-controller";

CallController::CallController(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
: userver::components::LoggableComponentBase(config, context),
  pg_{context.FindComponent<userver::components::Postgres>("postgres").GetCluster()}
{}

void CallController::Save(std::vector<models::Call> &&calls) {
    if (calls.empty()) return;
    try {
        auto trx = pg_->Begin(userver::storages::postgres::ClusterHostType::kMaster);
        for (const auto& call : calls) {
            trx.Execute(
                "INSERT INTO calls "
                "(id, status, started_at, finished_at, caller_number, callee_number, user_id) "
                "VALUES ($1, $2, $3, $4, $5, $6, $7) "
                "ON CONFLICT (id) DO UPDATE SET "
                "status=EXCLUDED.status, started_at=EXCLUDED.started_at, finished_at=EXCLUDED.finished_at, "
                "caller_number=EXCLUDED.caller_number, callee_number=EXCLUDED.callee_number, user_id=EXCLUDED.user_id;",
                call.id,
                call.status,
                call.started_at,
                call.finished_at,
                call.caller_number,
                call.callee_number,
                call.user_id
            );
        }
        trx.Commit();
    } catch (const std::exception& ex) {
        LOG_ERROR() << "CallController Save error: " << ex.what();
        throw;
    }
}

std::vector<models::Call> CallController::GetCalls(const std::vector<std::int64_t>& call_ids) {
    std::vector<models::Call> result;
    if (call_ids.empty()) return result;
    try {
        auto trx = pg_->Begin(userver::storages::postgres::ClusterHostType::kSlave);
        auto res = trx.Execute(
            "SELECT id, status, started_at, finished_at, caller_number, callee_number, user_id "
            "FROM calls WHERE id = ANY($1)", call_ids);

        for (const auto& row : res) {
            models::Call call;
            call.id = row["id"].As<std::int64_t>();
            call.status = row["status"].As<std::string>();
            call.started_at = row["started_at"].As<userver::storages::postgres::TimePointTz>();
            call.finished_at = row["finished_at"].As<userver::storages::postgres::TimePointTz>();
            call.caller_number = row["caller_number"].As<std::string>();
            call.callee_number = row["callee_number"].As<std::string>();
            call.user_id = row["user_id"].As<std::int64_t>();
            result.emplace_back(std::move(call));
        }
        trx.Commit();
    } catch (const std::exception& ex) {
        LOG_ERROR() << "CallController GetCalls error: " << ex.what();
        throw;
    }
    return result;
}

models::Call CallController::GetCall(std::int64_t call_id) {
    try {
        auto trx = pg_->Begin(userver::storages::postgres::ClusterHostType::kSlave);
        auto res = trx.Execute(
            "SELECT id, status, started_at, finished_at, caller_number, callee_number, user_id "
            "FROM calls WHERE id = $1", call_id);

        if (res.IsEmpty())
            throw std::runtime_error("Call not found for id=" + std::to_string(call_id));

        auto row = res.Front();
        models::Call call;
        call.id = row["id"].As<std::int64_t>();
        call.status = row["status"].As<std::string>();
        call.started_at = row["started_at"].As<userver::storages::postgres::TimePointTz>();
        call.finished_at = row["finished_at"].As<userver::storages::postgres::TimePointTz>();
        call.caller_number = row["caller_number"].As<std::string>();
        call.callee_number = row["callee_number"].As<std::string>();
        call.user_id = row["user_id"].As<std::int64_t>();
        trx.Commit();
        return call;
    } catch (const std::exception& ex) {
        LOG_ERROR() << "CallController GetCall error: " << ex.what();
        throw;
    }
}

}  // namespace call_flow_processor::components::controllers
