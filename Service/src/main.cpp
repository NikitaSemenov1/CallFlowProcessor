#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/clients/dns/component.hpp>
#include <userver/components/fs_cache.hpp>

#include "components/cdr_uploaders/cdr_upload_info.hpp"
#include "components/cdr_uploaders/cdr_uploader.hpp"
#include "components/cdr_uploaders/external_cdr_uploader.hpp"
#include "components/controllers/call_controller.hpp"
#include "components/controllers/call_event_controller.hpp"
#include "components/controllers/connection_controller.hpp"
#include "components/controllers/operator_controller.hpp"
#include "components/controllers/cdr_controller.hpp"
#include "components/data_fetchers/call_data_fetcher.hpp"
#include "components/data_fetchers/call_event_data_fetcher.hpp"
#include "components/data_fetchers/connection_data_fetcher.hpp"
#include "components/data_fetchers/operator_data_fetcher.hpp"
#include "handlers/statistics/calls/summary/handler.hpp"
#include "handlers/statistics/operators/handler.hpp"

int main(int argc, char* argv[]) {
  auto component_list = userver::components::MinimalServerComponentList()
    .Append<userver::server::handlers::Ping>()
    .Append<userver::components::TestsuiteSupport>()
    .Append<userver::server::handlers::TestsControl>()
    .Append<userver::components::HttpClient>()
    .Append<userver::components::Postgres>()
    .Append<userver::clients::dns::Component>()
    .Append<userver::components::FsCache>();

  component_list
    .Append<call_flow_processor::components::controllers::CallController>()
    .Append<call_flow_processor::components::controllers::CallEventController>()
    .Append<call_flow_processor::components::controllers::ConnectionController>()
    .Append<call_flow_processor::components::controllers::OperatorController>()
    .Append<call_flow_processor::components::controllers::CDRController>()

    .Append<call_flow_processor::components::data_fetchers::CallDataFetcher>()
    .Append<call_flow_processor::components::data_fetchers::CallEventDataFetcher>()
    .Append<call_flow_processor::components::data_fetchers::ConnectionDataFetcher>()
    .Append<call_flow_processor::components::data_fetchers::OperatorDataFetcher>()

    .Append<call_flow_processor::components::CDRUploadInfo>()
    .Append<call_flow_processor::components::CDRUploader>()
    .Append<call_flow_processor::components::ExternalCDRUploader>()

    .Append<call_flow_processor::handlers::StatisticsCallsSummaryHandler>()
    .Append<call_flow_processor::handlers::StatisticsOperatorsHandler>()
    ;

  return userver::utils::DaemonMain(argc, argv, component_list);
}
