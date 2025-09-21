DROP SCHEMA IF EXISTS call_flow_processor CASCADE;

CREATE SCHEMA IF NOT EXISTS call_flow_processor;

CREATE TABLE IF NOT EXISTS call_flow_processor.calls (
    call_id         BIGINT PRIMARY KEY,
    call_type       VARCHAR,
    scenario_id     VARCHAR,
    initiator_phone VARCHAR,
    media_record_id VARCHAR,
    initiated_at    TIMESTAMP,
    finished_at     TIMESTAMP,
    context         JSON
);

CREATE TABLE IF NOT EXISTS call_flow_processor.connections (
    connection_id   BIGINT PRIMARY KEY,
    call_id         BIGINT,
    phone           VARCHAR,
    initiated_at    TIMESTAMP,
    answered_at     TIMESTAMP,
    finished_at     TIMESTAMP,
    FOREIGN KEY (call_id) REFERENCES call_flow_processor.calls(call_id)
);

CREATE TABLE IF NOT EXISTS call_flow_processor.call_events (
    event_id    BIGINT PRIMARY KEY,
    call_id     BIGINT,
    event_type  VARCHAR,
    payload     JSON,
    FOREIGN KEY (call_id) REFERENCES call_flow_processor.calls(call_id)
);

CREATE TABLE IF NOT EXISTS call_flow_processor.finished_calls (
    call_id     BIGINT PRIMARY KEY,
    call_type   VARCHAR,
    scenario_id VARCHAR
);

CREATE TABLE IF NOT EXISTS call_flow_processor.cdr_upload_info (
    call_id             BIGINT,
    cdr_uploader_id     VARCHAR,
    upload_status       VARCHAR,
    created_at          TIMESTAMP,
    uploaded_at         TIMESTAMP,
    PRIMARY KEY (call_id, cdr_uploader_id),
    FOREIGN KEY (call_id) REFERENCES call_flow_processor.calls(call_id)
);

CREATE TABLE IF NOT EXISTS call_flow_processor.distlocks (
    key             VARCHAR PRIMARY KEY,
    owner           TEXT,
    expiration_time TIMESTAMP
);

CREATE TABLE IF NOT EXISTS call_flow_processor.data_fetchers (
    fetcher_id  VARCHAR PRIMARY KEY,
    cursor      BIGINT
);

CREATE TABLE IF NOT EXISTS call_flow_processor.operators (
    operator_id BIGINT PRIMARY KEY,
    name        VARCHAR NOT NULL,
    extension   VARCHAR NOT NULL,
    email       VARCHAR NOT NULL
);
