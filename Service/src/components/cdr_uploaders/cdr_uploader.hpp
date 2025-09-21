#pragma once

#include "cdr_uploader_base.hpp"
#include "models/cdr.hpp"
#include "components/controllers/cdr_controller.hpp"
#include "components/controllers/call_controller.hpp"
#include "components/controllers/call_event_controller.hpp"
#include "components/controllers/operator_controller.hpp"
#include "components/controllers/connection_controller.hpp"
#include "components/cdr_upload_info.hpp"

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <unordered_map>
#include <string>
#include <vector>

namespace call_flow_processor::components {

class CDRUploader final : public CDRUploaderBase<models::CDR> {
public:
    static constexpr const char* kName;

    CDRUploader(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& context);

protected:
    std::string GetId() override;
    std::vector<models::CDR> Collect(const std::vector<std::int64_t>& call_ids) override;
    void Upload(std::vector<models::CDR>&& data) override;

private:
    controllers::CDRController& cdr_controller_;
    controllers::CallController& call_controller_;
    controllers::CallEventController& call_event_controller_;
    controllers::OperatorController& operator_controller_;
    controllers::ConnectionController& connection_controller_;
};

} // namespace call_flow_processor::components
