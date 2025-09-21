import pytest
import json

@pytest.fixture
def mock_calls(mockserver):
    def _(_req):
        # One complete, one incomplete (missing in call_events)
        return mockserver.make_response(json.dumps([
            {
                "id": 100,
                "status": "COMPLETED",
                "started_at": "2024-06-18T12:00:00Z",
                "finished_at": "2024-06-18T12:05:00Z",
                "caller_number": "+79991112233",
                "callee_number": "54321",
                "user_id": 200,
            },
            {
                "id": 101,
                "status": "NO_ANSWER",
                "started_at": "2024-06-18T12:10:00Z",
                "finished_at": "2024-06-18T12:11:00Z",
                "caller_number": "+79991110000",
                "callee_number": "12345",
                "user_id": 201,
            },
        ]), 200)
    mockserver.json_handler("/calls")(_)
    return _

@pytest.fixture
def mock_operators(mockserver):
    def _(_req):
        return mockserver.make_response(json.dumps([
            {"operator_id": 200, "name": "Alice", "extension": "001", "email": "alice@test.com"},
            {"operator_id": 201, "name": "Bob", "extension": "002", "email": "bob@test.com"},
        ]), 200)
    mockserver.json_handler("/operators")(_)
    return _

@pytest.fixture
def mock_call_events(mockserver):
    def _(_req):
        return mockserver.make_response(json.dumps([
            {"event_id": 1, "call_id": 100, "event_type": "start", "payload": {}},
            {"event_id": 2, "call_id": 100, "event_type": "hangup", "payload": {}},
            # No events for call_id 101
        ]), 200)
    mockserver.json_handler("/call_events")(_)
    return _

@pytest.fixture
def mock_connections(mockserver):
    def _(_req):
        return mockserver.make_response(json.dumps([
            {"connection_id": 1, "call_id": 100, "phone": "+79991112233",
             "initiated_at": "2024-06-18T12:00:00Z",
             "answered_at": "2024-06-18T12:00:10Z",
             "finished_at": "2024-06-18T12:05:00Z"},
        ]), 200)
    mockserver.json_handler("/connections")(_)
    return _


@pytest.mark.usefixtures("mock_calls", "mock_operators", "mock_call_events", "mock_connections")
async def test_cdr_uploader_uploads_only_ready_calls(service_client, pgsql, testpoint):
    # Check that only call_id=100 is in cdrs table
    cdrs = await pgsql['db'].fetch('SELECT * FROM cdrs;')
    assert len(cdrs) == 1
    cdr = cdrs[0]
    assert cdr['call_id'] == '100'
    assert cdr['caller_number'] == "+79991112233"
    assert cdr['callee_number'] == "54321"

@pytest.mark.usefixtures("mock_calls", "mock_operators", "mock_call_events", "mock_connections")
async def test_cdr_uploader_skips_incomplete_data(service_client, pgsql):
    # Simulate call_id 102 with no operator OR connection, should be skipped
    await service_client.post('/admin/trigger-cdr-upload')

    cdrs = await pgsql['db'].fetch('SELECT * FROM cdrs;')
    # Still just one record for ready call_id=100
    assert len(cdrs) == 1
    assert cdrs[0]['call_id'] == '100'

@pytest.mark.usefixtures("mock_calls", "mock_operators", "mock_call_events", "mock_connections")
async def test_cdr_uploader_data_fields(service_client, pgsql):
    await service_client.post('/admin/trigger-cdr-upload')
    cdrs = await pgsql['db'].fetch('SELECT * FROM cdrs;')
    cdr = cdrs[0]
    assert cdr['duration_sec'] == 300  # 5min
    assert cdr['call_result'] == "COMPLETED"
    assert sorted(cdr['call_events']) == ["hangup", "start"]

@pytest.mark.parametrize(
    "skip_mock,should_upload",
    [
        (None, True),              # no skip, upload
        ("mock_call_events", False),  # missing events, skip
        ("mock_operators", False),    # missing operator, skip
    ]
)
async def test_cdr_uploader_various_missing_sources(skip_mock, should_upload, mock_calls, mock_operators,
                                                   mock_call_events, mock_connections, service_client, pgsql, request):
    if skip_mock:
        request.getfixturevalue(skip_mock)
    await service_client.post('/admin/trigger-cdr-upload')
    cdrs = await pgsql['db'].fetch('SELECT * FROM cdrs;')
    if should_upload:
        assert len(cdrs) == 1
    else:
        assert len(cdrs) == 0
