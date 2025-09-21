#pragma once

#include "data_fetcher_base.hpp"
#include "models/call_event.hpp"
#include "components/controllers/call_event_controller.hpp"
#include <userver/clients/http/client.hpp>
#include <string>
#include <vector>

namespace call_flow_processor::components::data_fetchers {

class CallEventDataFetcher final : public DataFetcherBase<models::CallEvent> {
public:
    CallEventDataFetcher(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

protected:
    std::string GetId() override;
    std::vector<models::CallEvent> Fetch(std::int64_t cursor) override;
    void Store(std::vector<models::CallEvent>&& data) override;

    userver::clients::http::Client& http_client_;
    controllers::CallEventController& call_event_controller_;
    std::string endpoint_;
    int fetch_limit_;
};

} // namespace call_flow_processor::components::data_fetchers
