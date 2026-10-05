from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from routers.chat import router

app = FastAPI(title="Planetarium AI", version="1.0.0")
app.add_middleware(CORSMiddleware, allow_origins=["*"], allow_methods=["POST","GET"], allow_headers=["*"])
app.include_router(router)

@app.get("/health")
async def health():
    return {"status": "ok"}
