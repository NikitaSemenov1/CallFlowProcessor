#include "call_data_fetcher.hpp"
#include <userver/clients/http/component.hpp>
#include <userver/formats/json/parse.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/logging/log.hpp>

namespace call_flow_processor::components::data_fetchers {

CallDataFetcher::CallDataFetcher(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
: DataFetcherBase<models::Call>(config, context),
  http_client_(context.FindComponent<userver::clients::http::Client>("http-client")),
  call_controller_(context.FindComponent<controllers::CallController>("call-controller")),
  endpoint_(config["source-endpoint"].As<std::string>()),
  fetch_limit_(config["fetch-limit"].As<int>(100))
{}

std::string CallDataFetcher::GetId() {
    return "call-data-fetcher";
}

std::vector<models::Call> CallDataFetcher::Fetch(std::int64_t cursor) {
    std::vector<models::Call> result;
    try {
        const auto url = endpoint_ + "?cursor=" + std::to_string(cursor) + "&limit=" + std::to_string(fetch_limit_);

        auto response = http_client_.CreateRequest()
            .get(url)
            .timeout(std::chrono::seconds(10))
            .perform();

        if (response->status_code != userver::clients::http::HttpStatus::kOk) {
            LOG_ERROR() << "Fetch failed from " << url << ", status: " << response->status_code;
            return result;
        }

        const auto json = userver::formats::json::FromString(response->body);

        if (!json.IsArray()) {
            LOG_ERROR() << "Fetch: json response is not array";
            return result;
        }

        for (const auto& item : json) {
            try {
                models::Call call;
                call.id = item["id"].As<std::int64_t>();
                call.status = item["status"].As<std::string>();
                call.started_at = item["started_at"].As<userver::storages::postgres::TimePointTz>();
                call.user_id = item["user_id"].As<std::int64_t>();
                result.push_back(std::move(call));
            } catch (const std::exception& ex){
                LOG_ERROR() << "Failed to parse call: " << ex.what();
            }
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Fetch failed: " << ex.what();
    }
    return result;
}

void CallDataFetcher::Store(std::vector<models::Call>&& data) {
    try {
        call_controller_.Save(std::move(data));
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Store failed: " << ex.what();
    }
}

}  // namespace call_flow_processor::components::data_fetchers
