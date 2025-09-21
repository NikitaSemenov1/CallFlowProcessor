#pragma once

#include <userver/storages/postgres/dist_lock_component_base.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <vector>
#include <string>
#include <chrono>
#include "components/cdr_upload_info.hpp"

namespace call_flow_processor::components {

template <class T>
class CDRUploaderBase : public userver::storages::postgres::DistLockComponentBase {
protected:
    CDRUploaderBase(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context)
        : userver::storages::postgres::DistLockComponentBase(config, context),
          upload_info_{context.FindComponent<CDRUploadInfo>("cdr-upload-info")}
    {}

    void DoWork() override {
        while (!userver::engine::current_task::IsCancelRequested()) {
            // 1. Find all finished calls and upsert as pending into cdr_upload_info
            const auto finished_calls = upload_info_.GetFinishedCallIds();
            for (const auto& id : finished_calls) {
                upload_info_.UpsertPending(GetId(), id);
            }

            // 2. Load pending call_ids to process
            const auto pending_call_ids = upload_info_.GetPendingCallIds(GetId(), GetBatchSize());

            // 3. Try to collect all needed data for each call_id and build CDRs
            auto output = Collect(pending_call_ids);

            // 4. Upload those we could build
            Upload(output);

            userver::engine::InterruptibleSleepFor(std::chrono::seconds(5));
        }
    }

    virtual std::string GetId() = 0;
    virtual std::vector<T> Collect(const std::vector<std::int64_t>& call_ids) = 0;
    virtual void Upload(std::vector<T>&&) = 0;

    CDRUploadInfo& upload_info_;
};

} // namespace call_flow_processor::components
