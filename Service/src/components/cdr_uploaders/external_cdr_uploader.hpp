#pragma once

#include "cdr_uploader_base.hpp"
#include "models/external_cdr.hpp"
#include "components/controllers/call_controller.hpp"
#include "components/controllers/call_event_controller.hpp"
#include "components/controllers/operator_controller.hpp"
#include "components/controllers/connection_controller.hpp"
#include "components/cdr_upload_info.hpp"

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <unordered_map>
#include <string>
#include <vector>

namespace call_flow_processor::components {

class ExternalCDRUploader final : public CDRUploaderBase<models::ExternalCDR> {
public:
    static constexpr const char* kName;

    ExternalCDRUploader(const userver::components::ComponentConfig& config,
                        const userver::components::ComponentContext& context);

protected:
    std::string GetId() override;
    std::vector<models::ExternalCDR> Collect(const std::vector<std::int64_t>& call_ids) override;
    void Upload(std::vector<models::ExternalCDR>&& data) override;

private:
    controllers::CallController& call_controller_;
    controllers::CallEventController& call_event_controller_;
    controllers::OperatorController& operator_controller_;
    controllers::ConnectionController& connection_controller_;
    userver::clients::http::Client& http_client_;
    std::string upload_url_;
};

} // namespace call_flow_processor::components
