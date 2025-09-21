#include "connection_controller.hpp"
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/database.hpp>

namespace call_flow_processor::components::controllers {

const char* ConnectionController::kName = "connection-controller";

ConnectionController::ConnectionController(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
: userver::components::LoggableComponentBase(config, context),
  pg_{context.FindComponent<userver::components::Postgres>("postgres").GetCluster()}
{}

void ConnectionController::Save(std::vector<models::Connection>&& connections) {
    if (connections.empty()) return;
    try {
        auto trx = pg_->Begin(userver::storages::postgres::ClusterHostType::kMaster);

        for (const auto& conn : connections) {
            trx.Execute(
                "INSERT INTO connections "
                "(connection_id, call_id, phone, initiated_at, answered_at, finished_at) VALUES "
                "($1, $2, $3, $4, $5, $6) "
                "ON CONFLICT(connection_id) DO UPDATE SET "
                "call_id=EXCLUDED.call_id, phone=EXCLUDED.phone, "
                "initiated_at=EXCLUDED.initiated_at, "
                "answered_at=EXCLUDED.answered_at, "
                "finished_at=EXCLUDED.finished_at;",
                conn.connection_id,
                conn.call_id,
                conn.phone,
                conn.initiated_at,
                conn.answered_at,
                conn.finished_at
            );
        }

        trx.Commit();
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Failed to save connections: " << ex.what();
        throw;
    }
}

std::vector<models::Connection> ConnectionController::GetConnections(const std::vector<std::int64_t>& connection_ids) {
    std::vector<models::Connection> result;
    if (connection_ids.empty()) return result;

    try {
        auto trx = pg_->Begin(userver::storages::postgres::ClusterHostType::kMaster);
        auto res = trx.Execute(
            "SELECT connection_id, call_id, phone, initiated_at, answered_at, finished_at "
            "FROM connections WHERE connection_id = ANY($1)", connection_ids);

        for (const auto& row : res) {
            models::Connection c;
            c.connection_id = row["connection_id"].As<std::int64_t>();
            c.call_id       = row["call_id"].As<std::int64_t>();
            c.phone         = row["phone"].As<std::string>();
            c.initiated_at  = row["initiated_at"].As<userver::storages::postgres::TimePointTz>();
            c.answered_at   = row["answered_at"].As<std::optional<userver::storages::postgres::TimePointTz>>();
            c.finished_at   = row["finished_at"].As<std::optional<userver::storages::postgres::TimePointTz>>();
            result.emplace_back(std::move(c));
        }
        trx.Commit();
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Failed to fetch connections: " << ex.what();
        throw;
    }
    return result;
}

} // namespace call_flow_processor::components::controllers
