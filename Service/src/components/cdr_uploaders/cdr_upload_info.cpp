#include "cdr_upload_info.hpp"
#include <userver/logging/log.hpp>

namespace call_flow_processor::components {

const char* CDRUploadInfo::kName = "cdr-upload-info";

CDRUploadInfo::CDRUploadInfo(const userver::components::ComponentConfig& config,
                             const userver::components::ComponentContext& context)
    : userver::components::LoggableComponentBase(config, context),
      pg_{context.FindComponent<userver::components::Postgres>("postgres").GetCluster()} {}

std::vector<std::int64_t> CDRUploadInfo::GetFinishedCallIds() const {
    std::vector<std::int64_t> res;
    try {
        auto result = pg_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT call_id FROM finished_calls"
        );
        for (const auto& row : result) {
            res.push_back(row["call_id"].As<std::int64_t>());
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "CDRUploadInfo::GetFinishedCallIds error: " << ex.what();
        throw;
    }
    return res;
}

void CDRUploadInfo::BatchStoreFinishedCalls(const std::vector<std::int64_t>& call_ids) {
    if (call_ids.empty()) return;
    try {
        auto trx = pg_->Begin(userver::storages::postgres::ClusterHostType::kMaster);
        trx.Execute(
            "INSERT INTO finished_calls (call_id) "
            "SELECT UNNEST($1::bigint[]) "
            "ON CONFLICT DO NOTHING;",
            call_ids
        );
        trx.Commit();
    } catch (const std::exception& ex) {
        LOG_ERROR() << "CDRUploadInfo::BatchStoreFinishedCalls error: " << ex.what();
        throw;
    }
}

void CDRUploadInfo::UpsertPending(const std::string& cdr_type, std::int64_t call_id) {
    try {
        pg_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO cdr_upload_info (cdr_type, call_id, upload_status) "
            "VALUES ($1, $2, 'pending') "
            "ON CONFLICT (cdr_type, call_id) DO NOTHING;",
            cdr_type, call_id
        );
    } catch (const std::exception& ex) {
        LOG_ERROR() << "CDRUploadInfo::UpsertPending error: " << ex.what();
        throw;
    }
}

std::vector<std::int64_t> CDRUploadInfo::GetPendingCallIds(const std::string& cdr_type, std::size_t limit) {
    std::vector<std::int64_t> res;
    try {
        auto result = pg_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT call_id FROM cdr_upload_info WHERE cdr_type = $1 AND upload_status = 'pending' LIMIT $2",
            cdr_type, static_cast<std::int64_t>(limit)
        );
        for (const auto& row : result) {
            res.push_back(row["call_id"].As<std::int64_t>());
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "CDRUploadInfo::GetPendingCallIds error: " << ex.what();
        throw;
    }
    return res;
}

void CDRUploadInfo::MarkUploaded(const std::string& cdr_type, std::int64_t call_id) {
    try {
        pg_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "UPDATE cdr_upload_info SET upload_status = 'uploaded', uploaded_at = now() "
            "WHERE cdr_type = $1 AND call_id = $2",
            cdr_type, call_id
        );
    } catch (const std::exception& ex) {
        LOG_ERROR() << "CDRUploadInfo::MarkUploaded error: " << ex.what();
        throw;
    }
}

} // namespace call_flow_processor::components
