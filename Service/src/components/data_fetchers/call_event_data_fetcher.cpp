#include "call_event_data_fetcher.hpp"
#include <userver/clients/http/component.hpp>
#include <userver/formats/json/parse.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/logging/log.hpp>

namespace call_flow_processor::components::data_fetchers {

CallEventDataFetcher::CallEventDataFetcher(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
: DataFetcherBase<models::CallEvent>(config, context),
  http_client_(context.FindComponent<userver::clients::http::Client>("http-client")),
  call_event_controller_(context.FindComponent<controllers::CallEventController>("call-event-controller")),
  endpoint_(config["source-endpoint"].As<std::string>()),
  fetch_limit_(config["fetch-limit"].As<int>(100))
{}

std::string CallEventDataFetcher::GetId() {
    return "call-event-data-fetcher";
}

std::vector<models::CallEvent> CallEventDataFetcher::Fetch(std::int64_t cursor) {
    std::vector<models::CallEvent> result;
    try {
        const auto url = endpoint_ + "?cursor=" + std::to_string(cursor)
            + "&limit=" + std::to_string(fetch_limit_);

        auto response = http_client_.CreateRequest()
            .get(url)
            .timeout(std::chrono::seconds(10))
            .perform();

        if (response->status_code != userver::clients::http::HttpStatus::kOk) {
            LOG_ERROR() << "CallEventDataFetcher fetch failed from " << url
                        << ", status: " << response->status_code;
            return result;
        }

        const auto json = userver::formats::json::FromString(response->body);

        if (!json.IsArray()) {
            LOG_ERROR() << "CallEventDataFetcher fetch: response is not array";
            return result;
        }

        for (const auto& item : json) {
            try {
                models::CallEvent ev;
                ev.event_id = item["event_id"].As<std::int64_t>();
                ev.call_id  = item["call_id"].As<std::int64_t>();
                ev.event_type = item["event_type"].As<std::string>();
                ev.payload = item["payload"];
                result.emplace_back(std::move(ev));
            } catch (const std::exception& ex) {
                LOG_ERROR() << "CallEventDataFetcher failed to parse row: " << ex.what();
            }
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "CallEventDataFetcher fetch failed: " << ex.what();
    }
    return result;
}

void CallEventDataFetcher::Store(std::vector<models::CallEvent>&& data) {
    try {
        call_event_controller_.Save(std::move(data));
    } catch (const std::exception& ex) {
        LOG_ERROR() << "CallEventDataFetcher store failed: " << ex.what();
    }
}

} // namespace call_flow_processor::components::data_fetchers
