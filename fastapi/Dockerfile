# --- STAGE 1: BUILDER ---
FROM python:3.12-bookworm AS builder

WORKDIR /app

# Installa le dipendenze in una cartella isolata
COPY requirements.txt requirements.lock.txt ./
RUN pip install --no-cache-dir --prefix=/install -r requirements.txt -c requirements.lock.txt


# --- STAGE 2: RUNTIME ---
FROM python:3.12-slim-bookworm

# Copia le dipendenze installate dallo stage builder
COPY --from=builder /install /usr/local

WORKDIR /app

# Copia il codice sorgente
COPY . .

# Crea cartella per i log
RUN mkdir -p /app/logs

# Esponiamo solo la porta interna (non sarà mappata all'host)
EXPOSE 8000

CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "8000", "--workers", "2"]
