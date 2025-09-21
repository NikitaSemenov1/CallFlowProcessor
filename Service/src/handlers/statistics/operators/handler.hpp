#pragma once

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <unordered_map>
#include <cmath>

#include "components/controllers/operator_controller.hpp"
#include "components/controllers/call_controller.hpp"
#include "models/operator.hpp"
#include "models/call.hpp"

namespace call_flow_processor::handlers {

class StatisticsOperatorsHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-statistics-operators";

  StatisticsOperatorsHandler(const userver::components::ComponentConfig& config,
                            const userver::components::ComponentContext& context)
      : userver::server::handlers::HttpHandlerBase(config, context),
        operator_controller_(context.FindComponent<components::controllers::OperatorController>("operator-controller")),
        call_controller_(context.FindComponent<components::controllers::CallController>("call-controller")) {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest&,
      userver::server::request::RequestContext&) const override {

    try {
      
      std::vector<call_flow_processor::models::Operator> operators = operator_controller_.GetAllOperators();
      
      std::unordered_map<std::int64_t, call_flow_processor::models::Operator> op_map;
      for (const auto& op : operators) op_map[op.operator_id] = op;

      std::vector<call_flow_processor::models::Call> calls = call_controller_.GetAllCallsWithFinishTime();

      struct OpStat {
        std::string name;
        int count = 0;
        double total_duration = 0.0;
      };
      std::unordered_map<std::int64_t, OpStat> stats;

      for (const auto& call : calls) {
        std::int64_t op_id = call.user_id;
        auto it = op_map.find(op_id);
        if (it == op_map.end()) continue; // skip calls without operator
        auto& stat = stats[op_id];
        stat.name = it->second.name;

        using namespace std::chrono;
        double duration = (call.finished_at - call.started_at).count();
        ++stat.count;
        stat.total_duration += duration;
      }

      userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kArray);
      for (const auto& [op_id, op] : op_map) {
        auto& stat = stats[op_id];
        double avg = (stat.count ? (stat.total_duration / stat.count) : 0);

        userver::formats::json::ValueBuilder ob;
        ob["operator_name"] = op.name;
        ob["call_count"] = stat.count;
        ob["avg_call_duration_seconds"] = std::round(avg);

        builder.PushBack(ob.ExtractValue());
      }

      return userver::formats::json::ToString(builder.ExtractValue());
    } catch (const std::exception& e) {
      LOG_ERROR() << "StatisticsOperatorsHandler exception: " << e.what();
      throw;
    }
  }

 private:
  components::controllers::OperatorController& operator_controller_;
  components::controllers::CallController& call_controller_;
};

}  // namespace call_flow_processor::handlers
