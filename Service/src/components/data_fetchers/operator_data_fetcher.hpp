#pragma once

#include "data_fetcher_base.hpp"
#include "models/operator.hpp"
#include "components/controllers/operator_controller.hpp"
#include <userver/clients/http/client.hpp>
#include <string>
#include <vector>

namespace call_flow_processor::components::data_fetchers {

class OperatorDataFetcher final : public DataFetcherBase<models::Operator> {
public:
    OperatorDataFetcher(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

protected:
    std::string GetId() override;
    std::vector<models::Operator> Fetch(std::int64_t cursor) override;
    void Store(std::vector<models::Operator>&& data) override;

    userver::clients::http::Client& http_client_;
    controllers::OperatorController& operator_controller_;
    std::string endpoint_;
    int fetch_limit_;
};

}  // namespace call_flow_processor::components::data_fetchers
