#pragma once

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>

#include "components/controllers/call_controller.hpp"
#include "models/call.hpp"

#include <cmath>

namespace call_flow_processor::handlers {

class StatisticsCallsSummaryHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-statistics-calls-summary";

  StatisticsCallsSummaryHandler(const userver::components::ComponentConfig& config,
                                const userver::components::ComponentContext& context)
      : userver::server::handlers::HttpHandlerBase(config, context),
        call_controller_(context.FindComponent<components::controllers::CallController>("call-controller")) {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest&,
      userver::server::request::RequestContext&) const override {
    try {
      const auto calls = call_controller_.GetAllCallsWithFinishTime();

      std::int64_t total_calls = static_cast<std::int64_t>(calls.size());
      std::int64_t answered_calls = 0;
      double total_duration_seconds = 0.0;

      for (const auto& call : calls) {
        if (call.status == "answered") ++answered_calls;

        using namespace std::chrono;
        auto duration = call.finished_at - call.started_at;
        total_duration_seconds += duration.count();
      }

      double avg_duration = 0.0;
      if (total_calls > 0)
        avg_duration = total_duration_seconds / total_calls;

      userver::formats::json::ValueBuilder builder;
      builder["total_calls"] = total_calls;
      builder["answered_calls"] = answered_calls;
      builder["avg_duration_seconds"] = std::round(avg_duration);
      builder["total_duration_seconds"] = std::round(total_duration_seconds);

      return userver::formats::json::ToString(builder.ExtractValue());
    } catch (const std::exception& e) {
      LOG_ERROR() << "StatisticsCallsSummaryHandler exception: " << e.what();
      throw;
    }
  }

 private:
  components::controllers::CallController& call_controller_;
};

}  // namespace call_flow_processor::handlers
