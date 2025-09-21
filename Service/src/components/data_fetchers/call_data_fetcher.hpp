#pragma once

#include "data_fetcher_base.hpp"
#include "models/call.hpp"
#include "components/controllers/call_controller.hpp"
#include <userver/clients/http/client.hpp>
#include <string>
#include <vector>

namespace call_flow_processor::components::data_fetchers {

class CallDataFetcher final : public DataFetcherBase<models::Call> {
public:
    CallDataFetcher(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

protected:
    std::string GetId() override;
    std::vector<models::Call> Fetch(std::int64_t cursor) override;
    void Store(std::vector<models::Call>&& data) override;

    userver::clients::http::Client& http_client_;
    controllers::CallController& call_controller_;
    std::string endpoint_;
    int fetch_limit_;
};

}  // namespace call_flow_processor::components::data_fetchers
