from fastapi import FastAPI, Request
from typing import List, Optional
from pydantic import BaseModel

app = FastAPI()

class CDR(BaseModel):
    call_id: int
    initiator_phone: str
    call_type: str
    scenario_id: str
    start_time: str
    end_time: str
    duration_seconds: int
    operator_id: Optional[int]
    operator_name: Optional[str]
    call_result: Optional[str]
    number_of_connections: int
    number_of_events: int

@app.post("/records")
async def receive_cdr_batch(cdrs: List[CDR]):
    print("\n--- Received CDR batch ---")
    for record in cdrs:
        print(record.json())
    print("--- End of batch ---\n")
    return {"status": "OK", "received": len(cdrs)}
