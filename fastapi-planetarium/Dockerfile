FROM python:3.11-slim

# Build tools per sentence-transformers
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Dipendenze prima del codice per sfruttare il layer cache
COPY requirements.txt requirements.lock.txt ./
RUN pip install --no-cache-dir --prefer-binary -r requirements.txt -c requirements.lock.txt

# Precarica il modello di embedding al build time
# Evita il download a runtime (latenza al primo avvio)
RUN python -c "from sentence_transformers import SentenceTransformer; SentenceTransformer('all-MiniLM-L6-v2')"

COPY . .

EXPOSE 8000

CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "8000", "--workers", "1"]
