#include "operator_controller.hpp"
#include <userver/logging/log.hpp>

namespace call_flow_processor::components::controllers {

const char* OperatorController::kName = "operator-controller";

OperatorController::OperatorController(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
: userver::components::LoggableComponentBase(config, context),
  pg_{context.FindComponent<userver::components::Postgres>("postgres").GetCluster()}
{}

void OperatorController::Save(std::vector<models::Operator>&& operators) {
    if (operators.empty()) return;
    try {
        auto trx = pg_->Begin(userver::storages::postgres::ClusterHostType::kMaster);
        for (const auto& op : operators) {
            trx.Execute(
                "INSERT INTO operators (operator_id, name, extension, email) "
                "VALUES ($1, $2, $3, $4) "
                "ON CONFLICT(operator_id) DO UPDATE "
                "SET name=EXCLUDED.name, extension=EXCLUDED.extension, email=EXCLUDED.email;",
                op.operator_id,
                op.name,
                op.extension,
                op.email
            );
        }
        trx.Commit();
    } catch (const std::exception& ex) {
        LOG_ERROR() << "OperatorController Save error: " << ex.what();
        throw;
    }
}

std::vector<models::Operator> OperatorController::GetOperators(const std::vector<std::int64_t>& operator_ids) {
    std::vector<models::Operator> result;
    if (operator_ids.empty()) return result;
    try {
        auto trx = pg_->Begin(userver::storages::postgres::ClusterHostType::kMaster);
        auto res = trx.Execute(
            "SELECT operator_id, name, extension, email FROM operators WHERE operator_id = ANY($1)",
            operator_ids);

        for (const auto& row : res) {
            models::Operator op;
            op.operator_id = row["operator_id"].As<std::int64_t>();
            op.name = row["name"].As<std::string>();
            op.extension = row["extension"].As<std::string>();
            op.email = row["email"].As<std::string>();
            result.emplace_back(std::move(op));
        }
        trx.Commit();
    } catch (const std::exception& ex) {
        LOG_ERROR() << "OperatorController GetOperators error: " << ex.what();
        throw;
    }
    return result;
}

std::vector<models::Operator> OperatorController::GetAllOperators() {
    std::vector<models::Operator> res;
    auto trx = pg_->Begin(userver::storages::postgres::ClusterHostType::kSlave);
    auto r = trx.Execute("SELECT operator_id, name, extension, email FROM operators");
    for (const auto& row : r) {
        models::Operator op;
        op.operator_id = row["operator_id"].As<std::int64_t>();
        op.name = row["name"].As<std::string>();
        op.extension = row["extension"].As<std::string>();
        op.email = row["email"].As<std::string>();
        res.emplace_back(std::move(op));
    }
    return res;
}

}  // namespace call_flow_processor::components::controllers
