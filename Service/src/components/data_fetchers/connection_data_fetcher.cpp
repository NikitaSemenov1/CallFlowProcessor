#include "connection_data_fetcher.hpp"
#include <userver/clients/http/component.hpp>
#include <userver/formats/json/parse.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/logging/log.hpp>
#include <optional>

namespace call_flow_processor::components::data_fetchers {

ConnectionDataFetcher::ConnectionDataFetcher(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
: DataFetcherBase<models::Connection>(config, context),
  http_client_(context.FindComponent<userver::clients::http::Client>("http-client")),
  connection_controller_(context.FindComponent<controllers::ConnectionController>("connection-controller")),
  endpoint_(config["source-endpoint"].As<std::string>()),
  fetch_limit_(config["fetch-limit"].As<int>(100))
{}

std::string ConnectionDataFetcher::GetId() {
    return "connection-data-fetcher";
}

std::vector<models::Connection> ConnectionDataFetcher::Fetch(std::int64_t cursor) {
    std::vector<models::Connection> result;
    try {
        const auto url = endpoint_ + "?cursor=" + std::to_string(cursor)
            + "&limit=" + std::to_string(fetch_limit_);

        auto response = http_client_.CreateRequest()
            .get(url)
            .timeout(std::chrono::seconds(10))
            .perform();

        if (response->status_code != userver::clients::http::HttpStatus::kOk) {
            LOG_ERROR() << "ConnectionDataFetcher fetch failed from " << url
                        << ", status: " << response->status_code;
            return result;
        }

        const auto json = userver::formats::json::FromString(response->body);

        if (!json.IsArray()) {
            LOG_ERROR() << "ConnectionDataFetcher fetch: response is not array";
            return result;
        }

        for (const auto& item : json) {
            try {
                models::Connection conn;
                conn.connection_id = item["connection_id"].As<std::int64_t>();
                conn.call_id       = item["call_id"].As<std::int64_t>();
                conn.phone         = item["phone"].As<std::string>();
                conn.initiated_at  = item["initiated_at"].As<userver::storages::postgres::TimePointTz>();
                conn.answered_at   = item["answered_at"].IsMissing() ? std::nullopt :
                    std::make_optional(item["answered_at"].As<userver::storages::postgres::TimePointTz>());
                conn.finished_at   = item["finished_at"].IsMissing() ? std::nullopt :
                    std::make_optional(item["finished_at"].As<userver::storages::postgres::TimePointTz>());
                result.emplace_back(std::move(conn));
            } catch (const std::exception& ex) {
                LOG_ERROR() << "Failed to parse connection row: " << ex.what();
            }
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "ConnectionDataFetcher fetch failed: " << ex.what();
    }
    return result;
}

void ConnectionDataFetcher::Store(std::vector<models::Connection>&& data) {
    try {
        connection_controller_.Save(std::move(data));
    } catch (const std::exception& ex) {
        LOG_ERROR() << "ConnectionDataFetcher store failed: " << ex.what();
    }
}

} // namespace call_flow_processor::components::data_fetchers
