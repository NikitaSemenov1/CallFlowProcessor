#include "operator_data_fetcher.hpp"
#include <userver/clients/http/component.hpp>
#include <userver/formats/json/parse.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/logging/log.hpp>

namespace call_flow_processor::components::data_fetchers {

OperatorDataFetcher::OperatorDataFetcher(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
: DataFetcherBase<models::Operator>(config, context),
  http_client_(context.FindComponent<userver::clients::http::Client>("http-client")),
  operator_controller_(context.FindComponent<controllers::OperatorController>("operator-controller")),
  endpoint_(config["source-endpoint"].As<std::string>()),
  fetch_limit_(config["fetch-limit"].As<int>(100))
{}

std::string OperatorDataFetcher::GetId() {
    return "operator-data-fetcher";
}

std::vector<models::Operator> OperatorDataFetcher::Fetch(std::int64_t cursor) {
    std::vector<models::Operator> result;
    try {
        const auto url = endpoint_ + "?cursor=" + std::to_string(cursor)
            + "&limit=" + std::to_string(fetch_limit_);

        auto response = http_client_.CreateRequest()
            .get(url)
            .timeout(std::chrono::seconds(10))
            .perform();

        if (response->status_code != userver::clients::http::HttpStatus::kOk) {
            LOG_ERROR() << "OperatorDataFetcher fetch failed from " << url
                        << ", status: " << response->status_code;
            return result;
        }

        const auto json = userver::formats::json::FromString(response->body);

        if (!json.IsArray()) {
            LOG_ERROR() << "OperatorDataFetcher fetch: response is not array";
            return result;
        }

        for (const auto& item : json) {
            try {
                models::Operator op;
                op.operator_id = item["operator_id"].As<std::int64_t>();
                op.name = item["name"].As<std::string>();
                op.extension = item["extension"].As<std::string>();
                op.email = item["email"].As<std::string>();
                result.emplace_back(std::move(op));
            } catch (const std::exception& ex) {
                LOG_ERROR() << "OperatorDataFetcher failed to parse row: " << ex.what();
            }
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "OperatorDataFetcher fetch failed: " << ex.what();
    }
    return result;
}

void OperatorDataFetcher::Store(std::vector<models::Operator>&& data) {
    try {
        operator_controller_.Save(std::move(data));
    } catch (const std::exception& ex) {
        LOG_ERROR() << "OperatorDataFetcher store failed: " << ex.what();
    }
}

}  // namespace call_flow_processor::components::data_fetchers
