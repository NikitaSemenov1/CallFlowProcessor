#pragma once

#include "data_fetcher_base.hpp"
#include "models/connection.hpp"
#include "components/controllers/connection_controller.hpp"
#include <userver/clients/http/client.hpp>
#include <string>
#include <vector>

namespace call_flow_processor::components::data_fetchers {

class ConnectionDataFetcher final : public DataFetcherBase<models::Connection> {
public:
    ConnectionDataFetcher(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

protected:
    std::string GetId() override;
    std::vector<models::Connection> Fetch(std::int64_t cursor) override;
    void Store(std::vector<models::Connection>&& data) override;

    userver::clients::http::Client& http_client_;
    controllers::ConnectionController& connection_controller_;
    std::string endpoint_;
    int fetch_limit_;
};

} // namespace call_flow_processor::components::data_fetchers
