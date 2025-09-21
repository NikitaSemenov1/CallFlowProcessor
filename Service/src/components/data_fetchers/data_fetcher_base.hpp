#pragma once

#include <userver/storages/postgres/dist_lock_component_base.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/database.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/datetime.hpp>
#include <chrono>
#include <thread>

namespace call_flow_processor::components::data_fetchers {

template <class T>
class DataFetcherBase : public userver::storages::postgres::DistLockComponentBase {
public:
    DataFetcherBase(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context)
        : userver::storages::postgres::DistLockComponentBase(config, context),
          pg_{context.FindComponent<userver::components::Postgres>("postgres").GetCluster()} {}

protected:
    virtual std::string GetId() = 0;
    virtual std::vector<T> Fetch(std::int64_t cursor) = 0;
    virtual void Store(std::vector<T>&& data) = 0;

    std::int64_t GetCursor() {
        try {
            auto res = pg_->Execute(
              userver::storages::postgres::ClusterHostType::kSlave,
              "SELECT cursor FROM call_flow_processor.data_fetchers WHERE id=$1;", GetId());

            if (res.IsEmpty())
                return 0; // default cursor if not set

            return res.AsSingleRow<std::int64_t>();
        } catch (const std::exception& e) {
            LOG_ERROR() << "Failed to get cursor: " << e.what();
            return 0;
        }
    }

    void UpdateCursor(std::int64_t cursor) {
        try {
            pg_->Execute(
              userver::storages::postgres::ClusterHostType::kMaster,
              "INSERT INTO call_flow_processor.data_fetchers(id, cursor) VALUES($1, $2) "
              "ON CONFLICT(id) DO UPDATE SET cursor=EXCLUDED.cursor;",
              GetId(), cursor);
        } catch (const std::exception& e) {
            LOG_ERROR() << "Failed to update cursor: " << e.what();
        }
    }

    void DoWork() override {
        LOG_INFO() << "Starting DataFetcher: " << GetId();
        while (!userver::engine::current_task::IsCancelRequested()) {
            auto cursor = GetCursor();
            auto data = Fetch(cursor);

            if (!data.empty()) {
                Store(std::move(data));
                UpdateCursor(GetNextCursor(cursor, data));
            }

            // Backoff to avoid spinning
            userver::engine::InterruptibleSleepFor(std::chrono::seconds(3));
        }
    }

    virtual std::int64_t GetNextCursor(std::int64_t /*current_cursor*/, const std::vector<T>& data) {
        if (data.empty()) return 0;
        std::int64_t max_cursor = 0;
        for (const auto& elem : data) {
            if (elem.id > max_cursor) max_cursor = elem.id;
        }
        return max_cursor;
    }

    userver::storages::postgres::ClusterPtr pg_;
};

}  // namespace call_flow_processor::components::data_fetchers
