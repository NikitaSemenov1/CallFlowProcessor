from fastapi import FastAPI, Query
from typing import Optional
from pydantic import BaseModel
import json
import os

app = FastAPI()

BASE_DIR = os.path.dirname(__file__)


class Call(BaseModel):
    call_id: int
    call_type: str
    scenario_id: str
    initiator_phone: str
    media_record_id: str
    initiated_at: str
    finished_at: str
    context: dict

class Connection(BaseModel):
    connection_id: int
    call_id: int
    phone: str
    initiated_at: str
    answered_at: str
    finished_at: str

class CallEvent(BaseModel):
    event_id: int
    call_id: int
    event_type: str
    payload: dict

class Operator(BaseModel):
    operator_id: int
    name: str
    extension: str
    email: str


def load_mock_data(filename, key):
    with open(os.path.join(BASE_DIR, 'data', filename), 'r') as f:
        data = json.load(f)
    return data[key]

CALLS = [Call(**o) for o in load_mock_data("calls.json", "calls")]
CONNECTIONS = [Connection(**o) for o in load_mock_data("connections.json", "connections")]
CALL_EVENTS = [CallEvent(**o) for o in load_mock_data("call_events.json", "call_events")]
OPERATORS = [Operator(**o) for o in load_mock_data("operators.json", "operators")]

def paginate(data, cursor: Optional[int], limit: int, id_key: str):
    data = sorted(data, key=lambda x: getattr(x, id_key))
    idx = 0
    if cursor is not None:
        for i, entity in enumerate(data):
            if getattr(entity, id_key) > cursor:
                idx = i
                break
        else:
            return {"results": [], "next_cursor": None}
    results = data[idx:idx+limit]
    next_cursor = getattr(results[-1], id_key) if len(results) == limit else None
    return {"results": [r.dict() for r in results], "next_cursor": next_cursor}

@app.get("/calls")
def list_calls(cursor: Optional[int] = Query(None), limit: int = Query(10, gt=0)):
    return paginate(CALLS, cursor, limit, "call_id")

@app.get("/connections")
def list_connections(cursor: Optional[int] = Query(None), limit: int = Query(10, gt=0)):
    return paginate(CONNECTIONS, cursor, limit, "connection_id")

@app.get("/call_events")
def list_call_events(cursor: Optional[int] = Query(None), limit: int = Query(10, gt=0)):
    return paginate(CALL_EVENTS, cursor, limit, "event_id")

@app.get("/operators")
def list_operators(cursor: Optional[int] = Query(None), limit: int = Query(10, gt=0)):
    return paginate(OPERATORS, cursor, limit, "operator_id")

