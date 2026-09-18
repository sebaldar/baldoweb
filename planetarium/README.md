# 🌌 Planetarium 3D

Un planetario interattivo in tempo reale, accessibile via browser, con motore astronomico ad alte prestazioni, sistema agentico AI e supporto giroscopio per smartphone.

---

## Punti di Forza e Originalità

- **Motore astronomico C++ nativo** — calcolo delle posizioni celesti eseguito lato server in C++, compilato come libreria condivisa (`libPlanetarium.so`), esposto via Node.js. Precisione e velocità non raggiungibili con librerie JavaScript lato client.
- **Sistema agentico AI multimodale** — il chatbot non si limita a rispondere: interpreta comandi in linguaggio naturale, muove il telescopio, recupera dati astronomici in tempo reale e li sintetizza vocalmente.
- **Architettura full-duplex WebSocket** — aggiornamento continuo della volta celeste via WebSocket, senza polling. La vista risponde in tempo reale ai cambiamenti di data, ora e posizione geografica.
- **Modalità "Punta e Scatta"** — funzione originale per smartphone: il giroscopio del dispositivo viene usato per puntare il retro della fotocamera verso una porzione di cielo e richiedere il rendering astronomico esatto di quella direzione.
- **Multi-provider LLM** — il sistema agentico supporta IONOS AI Hub (GPT-OSS-120B), Anthropic Claude, OpenAI e Google Gemini, selezionabili a runtime senza riavvio.
- **Deploy EU-compliant** — infrastruttura self-hosted su server europei (Hetzner Helsinki + Nuremberg), con provider LLM IONOS (Germania) come default per conformità GDPR.

---

## Funzionalità Principali

- Rendering 3D della volta celeste con Three.js, proiezione sferica configurable
- Stelle fisse, pianeti del sistema solare, costellazioni e asterismi
- Navigazione per punti cardinali, zenit, e coordinate Az/Alt precise
- Ricerca e puntamento su oggetti celesti per nome
- Viaggi nel tempo: modifica di data e ora con aggiornamento immediato delle posizioni
- Selezione della posizione di osservazione via mappa interattiva (OpenLayers)
- Chatbot AI con riconoscimento vocale (Web Speech API) e sintesi vocale
- Console di debug integrata con invio comandi diretti al backend
- Responsive: ottimizzato per desktop e smartphone

---

## Architettura

```
┌─────────────────────────────────────────────────────┐
│                    CLIENT (Browser)                 │
│  Three.js (WebGL) · WebSocket · DeviceOrientation  │
│  Web Speech API · OpenLayers · PWA-ready           │
└─────────────────┬───────────────────────────────────┘
                  │ WSS (WebSocket Secure)
                  │ via Nginx SSL reverse proxy (Hetzner Nuremberg)
                  │ WireGuard VPN tunnel
                  ▼
┌─────────────────────────────────────────────────────┐
│              Node.js WebSocket Gateway              │
│  Smistamento messaggi · Gestione sessioni          │
│  Bridge HTTP verso FastAPI                         │
└─────────────────┬───────────────────────────────────┘
                  │ HTTP interno
                  ▼
┌─────────────────────────────────────────────────────┐
│              FastAPI (Python)                       │
│  LangGraph Agentic Pipeline · RAG · Multi-LLM      │
│  Endpoint /api/session · SSE streaming             │
└──────────┬──────────────────────────┬───────────────┘
           │                          │
           ▼                          ▼
┌──────────────────┐      ┌──────────────────────────┐
│  C++ Engine      │      │  Qdrant Vector Store     │
│  libPlanetarium  │      │  Enciclopedia astronomica│
│  .so             │      │  RAG knowledge base      │
│  Posizioni       │      └──────────────────────────┘
│  astronomiche    │
│  ad alta         │
│  precisione      │
└──────────────────┘
```

### Infrastruttura

| Componente | Tecnologia |
|---|---|
| Server applicativo | Hetzner CAX21 ARM64 Ubuntu — **baldoweb** |
| Reverse proxy SSL | Hetzner CX23 Nginx + Certbot (Nuremberg) |
| VPN inter-datacenter | WireGuard |
| Containerizzazione | Docker / Docker Compose |
| Database vettoriale | Qdrant (self-hosted) |

---

## Stack Tecnologico

### Backend
- **Python / FastAPI** — API REST e orchestrazione agente
- **LangGraph** — pipeline agentica con gestione stato e tool calling
- **Node.js** — WebSocket gateway e bridge verso il motore C++
- **C++ / libPlanetarium.so** — motore astronomico nativo con binding Node.js
- **Qdrant** — ricerca vettoriale semantica per la knowledge base astronomica

### Frontend
- **Three.js (r128)** — rendering WebGL della volta celeste
- **WebSocket API** — comunicazione bidirezionale in tempo reale
- **OpenLayers** — mappa interattiva per selezione posizione geografica
- **Web Speech API** — riconoscimento vocale e sintesi vocale in italiano
- **DeviceOrientation API + Wake Lock API** — giroscopio e schermo sempre attivo su mobile

### LLM Providers
- **IONOS AI Hub** (GPT-OSS-120B) — default, EU-hosted, privacy-compliant
- **Anthropic Claude** — avanzato
- **OpenAI GPT-4o-mini**
- **Google Gemini**

---

## Sistema Agentico AI

Il cuore intelligente del planetario è una pipeline **LangGraph** che trasforma il linguaggio naturale in azioni concrete sul telescopio virtuale.

### Capacità dell'Agente

**Risposta informativa con RAG**
L'utente chiede *"Parlami della nebulosa di Orione"* — l'agente recupera i documenti rilevanti dalla knowledge base astronomica su Qdrant tramite ricerca semantica e sintetizza una risposta contestualizzata.

**Comando diretto**
L'utente scrive o dice *"Punta verso Saturno"* — l'agente interpreta l'intento, calcola la posizione attuale del pianeta tramite il motore C++ (usando data, ora e coordinate GPS dell'osservatore), e invia il comando di navigazione al WebSocket. Il telescopio si muove.

**Queries temporali in tempo reale**
*"Dov'è la Luna adesso?"* — l'agente combina l'ora corrente, la posizione GPS del dispositivo e il motore astronomico per restituire azimut e altitudine precisi, aggiornati al secondo.

### Interazione Vocale

Il chatbot supporta input e output vocale in italiano:
- **Microfono**: riconoscimento vocale via Web Speech API — parla liberamente
- **Sintesi vocale**: le risposte vengono lette ad alta voce con voce italiana

---

## Modalità "Punta e Scatta" 📱🔭

Funzione originale pensata per l'uso outdoor con smartphone. Permette di puntare fisicamente il dispositivo verso una porzione di cielo e richiedere al motore il rendering astronomico esatto di quella direzione.

### Come Funziona

1. **Attivazione** — tocca il pulsante **🔭 Punta il Cielo** nel pannello dati. Su iOS viene richiesto esplicitamente il permesso di accesso al giroscopio.

2. **Interfaccia semplificata** — all'attivazione spariscono automaticamente tutti gli elementi non necessari (chatbot, selettore AI, stato connessione, guida), lasciando il canvas libero. Il pannello dati si chiude. Lo schermo rimane sempre acceso grazie alla **Wake Lock API**.

3. **Orientamento in tempo reale** — un HUD semitrasparente in alto mostra azimut e altitudine aggiornati in tempo reale mentre muovi il dispositivo:
   ```
   Az: 127.3°  |  Alt: 34.7°
   ```
   Un mirino centrale indica il punto di puntamento. Il riferimento direzionale è sempre il **lato superiore del dispositivo**, indipendentemente dall'inclinazione.

4. **Scatto** — tocca il grande pulsante arancione 📸 in basso al centro (raggiungibile con il pollice mentre tieni il telefono con entrambe le mani). Un flash visivo conferma la cattura.

5. **Rendering** — az e alt vengono inviati al backend (`view <az> <alt>`). Il motore C++ calcola le stelle e i pianeti visibili in quel cono di vista e aggiorna il rendering sul canvas.

6. **Uscita** — il pulsante **✕ Esci** in basso a destra ripristina la modalità normale con tutti i controlli.

### Algoritmo di Orientamento

La conversione giroscopio → coordinate celesti usa la **formula W3C ufficiale** per il vettore di puntamento del retro del dispositivo, con correzione continua del rollio (gamma) pesata sull'inclinazione:

```javascript
// Vettore direzione retro dispositivo (W3C)
Vx = -cos(α)·sin(γ) - sin(α)·sin(β)·cos(γ)
Vy = -sin(α)·sin(γ) + cos(α)·sin(β)·cos(γ)
az = atan2(Vx, Vy)

// Altitudine
alt = -asin(cos(β)·cos(γ))
```

L'azimut usa `deviceorientationabsolute` (Android Chrome) per riferirsi al **nord geografico reale**, con fallback su `deviceorientation` per iOS.

> **Nota:** Prima dell'uso outdoor si consiglia la calibrazione del magnetometro — aprire Google Maps e muovere il telefono a figura di 8 per 10-15 secondi.

---

## Avvio e Deploy

Il progetto gira interamente in Docker su **baldoweb**:

```bash
docker compose up -d
```

Il traffico HTTPS arriva sull'istanza Nginx (Nuremberg), attraversa il tunnel WireGuard e raggiunge il Node.js gateway su baldoweb (Helsinki). I certificati SSL sono gestiti da Certbot con rinnovo automatico.

---

## Licenza

Progetto privato — tutti i diritti riservati.
