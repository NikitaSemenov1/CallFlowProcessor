import pytest
import json

@pytest.fixture
def mock_calls(mockserver):
    def _(_req):
        return mockserver.make_response(json.dumps([
            {
                "id": 200,
                "status": "COMPLETED",
                "started_at": "2024-06-18T13:00:00Z",
                "finished_at": "2024-06-18T13:05:40Z",
                "caller_number": "+19998887766",
                "callee_number": "88888",
                "user_id": 300,
            },
        ]), 200)
    mockserver.json_handler("/calls")(_)
    return _

@pytest.fixture
def mock_operators(mockserver):
    def _(_req):
        return mockserver.make_response(json.dumps([
            {"operator_id": 300, "name": "Charlie", "extension": "100", "email": "charlie@test.com"},
        ]), 200)
    mockserver.json_handler("/operators")(_)
    return _

@pytest.fixture
def mock_call_events(mockserver):
    def _(_req):
        return mockserver.make_response(json.dumps([
            {"event_id": 11, "call_id": 200, "event_type": "answered", "payload": {}},
            {"event_id": 12, "call_id": 200, "event_type": "hangup", "payload": {}},
        ]), 200)
    mockserver.json_handler("/call_events")(_)
    return _

@pytest.fixture
def mock_connections(mockserver):
    def _(_req):
        return mockserver.make_response(json.dumps([
            {"connection_id": 2, "call_id": 200, "phone": "+19998887766",
             "initiated_at": "2024-06-18T13:00:00Z",
             "answered_at": "2024-06-18T13:00:10Z",
             "finished_at": "2024-06-18T13:05:40Z"},
        ]), 200)
    mockserver.json_handler("/connections")(_)
    return _

@pytest.fixture
def mock_external_records(mockserver):
    received = []
    @mockserver.json_handler("/records")
    def handler(request):
        data = request.json
        received.append(data)
        return {}
    return received

@pytest.mark.usefixtures("mock_calls", "mock_operators", "mock_call_events", "mock_connections", "mock_external_records")
async def test_external_cdr_uploader_uploads_ready_records(service_client, mock_external_records, pgsql):
    await service_client.post('/admin/trigger-external-cdr-upload')
    assert len(mock_external_records) == 1
    records = mock_external_records[0]
    assert isinstance(records, list)
    assert len(records) == 1
    rec = records[0]
    assert rec['call_id'] == '200'
    assert rec['caller_number'] == "+19998887766"
    assert rec['operator_id'] == "300"
    assert rec['operator_name'] == "Charlie"
    assert rec['agent_status'] == "ANSWERED"
    assert rec['wait_sec'] == 10
    assert rec['talk_sec'] == 330  # 13:05:40 - 13:00:10

@pytest.mark.usefixtures("mock_calls", "mock_operators", "mock_call_events", "mock_connections", "mock_external_records")
async def test_external_cdr_uploader_skips_incomplete_data(service_client, mockserver, mock_external_records, pgsql):
    # Remove call_events for the call (simulate incomplete)
    mockserver.json_handler("/call_events")(lambda _req: mockserver.make_response(json.dumps([]), 200))
    await service_client.post('/admin/trigger-external-cdr-upload')
    assert len(mock_external_records) == 0  # nothing uploaded

@pytest.mark.usefixtures("mock_calls", "mock_operators", "mock_call_events", "mock_connections", "mock_external_records")
async def test_external_cdr_uploader_fields_business_logic(service_client, mock_external_records, pgsql):
    await service_client.post('/admin/trigger-external-cdr-upload')
    # Check each uploaded field
    rec = mock_external_records[0][0]
    assert rec['call_id'] == '200'
    assert rec['agent_status'] == "ANSWERED"
    assert isinstance(rec['wait_sec'], int)
    assert isinstance(rec['talk_sec'], int)
    assert rec['end_reason'] == "COMPLETED" or rec['end_reason'] == "NO_ANSWER"

@pytest.mark.parametrize(
    "patch,should_upload",
    [
        (lambda mocks: mocks, True),               # All present, upload happens
        (lambda mocks: mocks.update(operators=False), False),  # No operators, skip
        (lambda mocks: mocks.update(calls=False), False),      # No calls, skip
    ]
)
async def test_external_cdr_uploader_various_sources(
    service_client,
    mock_calls, mock_operators, mock_call_events, mock_connections, mock_external_records,
    patch, should_upload, mockserver
):
    # Remove specified source
    mocks = {
        'calls': mock_calls,
        'operators': mock_operators,
        'call_events': mock_call_events,
        'connections': mock_connections,
    }
    patch(mocks)
    await service_client.post('/admin/trigger-external-cdr-upload')
    if should_upload:
        assert len(mock_external_records) == 1
    else:
        assert len(mock_external_records) == 0
