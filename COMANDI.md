# Documentazione Comandi WebSocket Server

Documentazione completa dei comandi disponibili per il server WebSocket astronomico.

## Formato dei Comandi

I comandi possono essere inviati in due formati:

1. **Formato XML**:
```xml
<root azione="command" data="nome_comando parametri..." />
```

2. **Formato testo semplice**:
```
nome_comando parametri...
```

---

## Comandi di Interazione Mouse

### `mouse`
**Sintassi**: `mouse <button> <name> <x> <y> <z> <distance> <type>`

**Descrizione**: Gestisce i click del mouse sugli oggetti celesti.

**Parametri**:
- `button`: 0 (sinistro), 1 (centrale), 2 (destro)
- `name`: Nome dell'oggetto (`earth`, `sky`, `sun`, `moon`, `mercury`, `venus`, `mars`, `jupiter`, `saturn`, `uranus`)
- `x`, `y`, `z`: Coordinate 3D del punto cliccato
- `distance`: Distanza dal punto di vista
- `type`: Tipo di evento (`down`, `up`, `move`)

**Risultati**:
- Per `earth`: Calcola latitudine/longitudine del punto, cerca città vicine (GeoNames API)
- Per `sky`: Calcola AR/Dec, altezza/azimut, identifica costellazioni e stelle
- Per `sun`: Mostra coordinate eclittiche/equatoriali, altezza sull'orizzonte
- Per `moon`: Calcola coordinate selenografiche, elongazione dal sole
- Per pianeti: Mostra coordinate equatoriali, altezza, azimut, costellazione

**Output XML**:
```xml
<results>
  <terminal>Informazioni testuali...</terminal>
  <dialog>HTML con dati dettagliati</dialog>
  <data>Aggiornamenti UI</data>
</results>
```

---

### `mouse_move`
**Sintassi**: `mouse_move <button> <name> <x> <y> <z> <distance> <type>`

**Descrizione**: Gestisce il movimento del mouse (drag).

**Risultati**: Aggiorna altezza e azimut se il mouse è premuto sulla vista del cielo.

---

### `mouse_dblclick`
**Sintassi**: `mouse_dblclick <button> <name> <x> <y> <z> <distance> <type>`

**Descrizione**: Gestisce il doppio click per cambiare punto di vista.

**Risultati**:
- Su `earth`: Imposta posizione osservatore su quel punto
- Su `sky`: Centra la camera su quelle coordinate celesti
- Su `moon`: Centra la vista sulla Luna

---

## Comandi di Osservazione

### `observe`
**Sintassi**: 
- `observe <longitudine> <latitudine> [direzione]`
- `observe` (usa coordinate correnti)

**Descrizione**: Imposta il punto di osservazione sulla Terra.

**Parametri**:
- `longitudine`: Longitudine in gradi (-180 a 180)
- `latitudine`: Latitudine in gradi (-90 a 90)
- `direzione`: `N`, `S`, `E`, `W`, `NE`, `NW`, `SE`, `SW`, `Z` (zenit), `sun`, `moon`, ecc.

**Risultati**: Imposta la posizione dell'osservatore e la direzione di vista.

**Output**:
```xml
<results>
  <terminal>Latitudine X Longitudine Y Distanza dal centro Z look at [direzione]</terminal>
</results>
```

---

### `lookfrom`
**Sintassi**: `lookfrom <luogo> <longitudine> <latitudine> [verso]`

**Descrizione**: Imposta il punto di vista (può essere `earth`, `moon`, ecc.).

**Parametri**:
- `luogo`: `earth`, `moon`, ecc.
- `longitudine`, `latitudine`: Coordinate
- `verso`: Direzione verso cui guardare

**Risultati**: Interroga GeoNames API per timezone, alba/tramonto.

**Output**:
```xml
<results>
  <terminal>Informazioni timezone e località</terminal>
  <data>
    <input id="latitudine" value="..." />
    <input id="longitudine" value="..." />
  </data>
</results>
```

---

### `lookat`
**Sintassi**: `lookat <oggetto>`

**Descrizione**: Punta la camera verso un oggetto celeste.

**Parametri**:
- `oggetto`: `earth`, `moon`, `sun`, `sky`, o nome pianeta

**Risultati**: Calcola posizione camera ottimale per l'oggetto.

---

### `view`
**Sintassi**: `view <direzione>`

**Descrizione**: Orienta la vista verso una direzione cardinale (solo da Terra).

**Parametri**:
- `direzione`: `N`, `S`, `E`, `W` (O), `NE`, `NW`, `SE`, `SW`, `Z` (zenit)

**Risultati**: Ruota la camera nella direzione specificata.

---

### `sky`
**Sintassi**: `sky <ascensione_retta> <declinazione>`

**Descrizione**: Punta la camera verso coordinate celesti specifiche.

**Parametri**:
- `ascensione_retta`: In ore (0-24)
- `declinazione`: In gradi (-90 a 90)

**Risultati**: Centra la vista sulle coordinate AR/Dec specificate.

**Output**:
```xml
<results>
  <renderer>
    <camera>
      <lookat x="..." y="..." z="..." />
    </camera>
    <primitive type="point" name="center">
      <vertice>x y z</vertice>
      <material size="5" color="0x0000ff" type="point" />
    </primitive>
  </renderer>
</results>
```

---

### `move`
**Sintassi**: `move <delta_altezza> <delta_azimut>`

**Descrizione**: Sposta la vista incrementalmente.

**Parametri**:
- `delta_altezza`: Variazione altezza in gradi
- `delta_azimut`: Variazione azimut in gradi

**Risultati**: Aggiorna altezza e azimut della vista.

---

## Comandi di Visualizzazione

### `zoom`
**Sintassi**: 
- `zoom <valore>`
- `zoom +` (raddoppia)
- `zoom -` (dimezza)

**Descrizione**: Controlla lo zoom della camera.

**Parametri**:
- `valore`: Fattore di zoom (deve essere > 0)

**Risultati**: Aggiorna lo zoom della camera e scala inversamente la sky sphere.

**Output**:
```xml
<results>
  <renderer name="solar">
    <primitive name="sky" type="sphere">
      <scale x="..." y="..." z="..." />
    </primitive>
  </renderer>
  <data>
    <span value="..." id="span_zoom" />
    <ui value="..." id="zoom" type="slider" />
  </data>
  <terminal>Camera zoom set to X (sky scale: Y)</terminal>
</results>
```

---

### `fov`
**Sintassi**: `fov <valore>`

**Descrizione**: Imposta il campo visivo (Field of View) della camera.

**Parametri**:
- `valore`: Angolo FOV in gradi (tipicamente 30-120)

**Risultati**: Modifica l'angolo di visione della camera.

---

### `rotation`
**Sintassi**: `rotation <angolo>`

**Descrizione**: Ruota la vista attorno all'asse verticale.

**Parametri**:
- `angolo`: Rotazione in gradi

**Risultati**: Modifica il vettore `up` della camera.

---

### `orbitControl`
**Sintassi**: `orbitControl <0|1>`

**Descrizione**: Attiva/disattiva i controlli orbitali.

**Parametri**:
- `1`: Attiva controlli orbitali (refresh automatico ogni ora)
- `0`: Disattiva controlli orbitali

**Output**:
```xml
<results>
  <renderer name="solar">
    <orbitControls /> <!-- o <orbitControls><remove /></orbitControls> -->
  </renderer>
</results>
```

---

## Comandi Griglia Celeste

### `constellation`
**Sintassi**: `constellation <codice> [hidden]`

**Descrizione**: Mostra/nasconde i confini di una costellazione.

**Parametri**:
- `codice`: Codice IAU della costellazione (es. `AND`, `ORI`, `UMA`)
- `hidden`: Opzionale, nasconde se presente

**Risultati**: Disegna/rimuove i bordi della costellazione.

**Output**:
```xml
<results>
  <renderer>
    <!-- Coordinate confini costellazione -->
  </renderer>
</results>
```

---

### `asterism`
**Sintassi**: `asterism <codice> [hidden]`

**Descrizione**: Mostra/nasconde l'asterismo (linee tra stelle) di una costellazione.

**Parametri**:
- `codice`: Codice IAU della costellazione
- `hidden`: Opzionale, nasconde se presente

**Risultati**: Disegna/rimuove le linee dell'asterismo.

---

### `ecliptic` / `eclittica`
**Sintassi**: `ecliptic <true|false>`

**Descrizione**: Mostra/nasconde la linea dell'eclittica.

**Parametri**:
- `true`: Mostra eclittica
- `false`: Nasconde eclittica

**Output**:
```xml
<results>
  <data>
    <input value="1" id="show_ecliptic" type="checkbox" />
  </data>
  <renderer>
    <!-- Tracciato eclittica -->
  </renderer>
</results>
```

---

### `equatorial` / `equatoriale`
**Sintassi**: `equatorial <true|false>`

**Descrizione**: Mostra/nasconde la griglia equatoriale.

**Parametri**:
- `true`: Mostra griglia
- `false`: Nasconde griglia

---

### `horizont`
**Sintassi**: `horizont <true|false>`

**Descrizione**: Mostra/nasconde la linea dell'orizzonte.

**Parametri**:
- `true`: Mostra orizzonte
- `false`: Nasconde orizzonte

**Output**:
```xml
<results>
  <data>
    <input value="1" id="show_horizont" type="checkbox" />
  </data>
  <renderer>
    <!-- Tracciato orizzonte -->
  </renderer>
</results>
```

---

### `axis`
**Sintassi**: `axis <true|false>`

**Descrizione**: Mostra/nasconde gli assi cartesiani.

**Parametri**:
- `true`: Mostra assi
- `false`: Nasconde assi

---

### `hit`
**Sintassi**: `hit <ascensione_retta> <declinazione>`

**Descrizione**: Identifica oggetti celesti in coordinate specifiche.

**Parametri**:
- `ascensione_retta`: In ore (0-24)
- `declinazione`: In gradi (-90 a 90)

**Risultati**: Restituisce stelle/oggetti Messier nelle vicinanze.

---

## Comandi Temporali

### `date`
**Sintassi**: `date <dd-mm-yyyy> <hh:mm:ss> [UT|DT]`

**Descrizione**: Imposta la data e ora di simulazione.

**Parametri**:
- `dd-mm-yyyy`: Data
- `hh:mm:ss`: Ora (opzionale, default 00:00:00)
- `UT|DT`: Tempo Universale o Tempo Dinamico (opzionale, default UT)

**Risultati**: Aggiorna la data di riferimento e ricalcola tutte le posizioni celesti.

**Output**:
```xml
<results>
  <terminal>Date set to [data] jd [julian_day]</terminal>
  <data>
    <input type="text" id="date" value="dd-mm-yyyy" />
    <input type="text" id="time" value="hh:mm" />
  </data>
</results>
```

---

### `simulation`
**Sintassi**: `simulation <fattore>`

**Descrizione**: Imposta la velocità di simulazione temporale.

**Parametri**:
- `fattore`: Moltiplicatore del tempo reale (es. 100 = 100x più veloce)

**Risultati**: Accelera/rallenta il passaggio del tempo nella simulazione.

**Output**:
```xml
<results>
  <data>
    <span value="X" id="span_simul" />
    <ui value="X" id="simul" type="slider" />
  </data>
  <terminal>Simulation set to X</terminal>
</results>
```

---

### `execute_loop`
**Sintassi**: `execute_loop <true|false|on|off>`

**Descrizione**: Attiva/disattiva il loop di aggiornamento continuo.

**Parametri**:
- `true`/`on`: Attiva loop
- `false`/`off`: Disattiva loop

---

### `refresh`
**Sintassi**: `refresh <secondi>`

**Descrizione**: Imposta l'intervallo di refresh automatico.

**Parametri**:
- `secondi`: Intervallo in secondi (0 = disabilitato)

**Risultati**: Aggiorna automaticamente la scena ogni N secondi.

---

## Comandi Calcoli Astronomici

### `sun_rise`
**Sintassi**: `sun_rise`

**Descrizione**: Calcola l'orario dell'alba per la posizione corrente.

**Requisiti**: Deve essere impostato un punto di osservazione sulla Terra.

**Risultati**: Restituisce data/ora dell'alba in formato UT.

**Output**:
```xml
<results>
  <terminal>[data_alba] ([julian_day]) [ora_locale] [sideral_time]</terminal>
</results>
```

---

### `angle`
**Sintassi**: `angle [julian_day]`

**Descrizione**: Calcola l'angolo del sole sull'orizzonte.

**Parametri**:
- `julian_day`: Giorno giuliano (opzionale, usa corrente se omesso)

**Requisiti**: Posizione osservatore sulla Terra.

**Risultati**: Angolo in gradi del sole sopra/sotto l'orizzonte.

---

### `equinox`
**Sintassi**: `equinox [anno]`

**Descrizione**: Calcola le date degli equinozi.

**Parametri**:
- `anno`: Anno (opzionale, usa corrente se omesso)

**Risultati**: Date/ore degli equinozi di marzo e settembre in DT e UT.

**Output**:
```
March equinox DT [data] UT [data]
September equinox DT [data] UT [data]
```

---

### `solstice`
**Sintassi**: `solstice [anno]`

**Descrizione**: Calcola le date dei solstizi.

**Parametri**:
- `anno`: Anno (opzionale, usa corrente se omesso)

**Risultati**: Date/ore dei solstizi di giugno e dicembre in DT e UT.

**Output**:
```
June solstice DT [data] UT [data]
December solstice DT [data] UT [data]
```

---

### `tropical` / `tropic`
**Sintassi**: `tropical year [anno]`

**Descrizione**: Calcola la durata dell'anno tropico.

**Parametri**:
- `anno`: Anno di riferimento (opzionale)

**Risultati**: Durata in giorni dell'anno tropico (tra due equinozi vernali).

---

### `precession`
**Sintassi**: `precession <x> <y> <z>`

**Descrizione**: Calcola la precessione degli equinozi per un punto.

**Parametri**:
- `x`, `y`, `z`: Coordinate 3D

**Risultati**: Coordinate corrette per la precessione al momento corrente.

---

## Comandi Oggetti Celesti

### `<nome_pianeta>`
**Sintassi**: 
- `<nome> [show|hide|hidden]`
- `<nome>` (mostra informazioni)

**Descrizione**: Controlla la visibilità o mostra info di un corpo celeste.

**Pianeti supportati**: `mercury`, `venus`, `mars`, `jupiter`, `saturn`, `uranus`, `earth`, `moon`, `sun`

**Con parametro**:
- `show`: Mostra il corpo
- `hide`/`hidden`: Nasconde il corpo

**Senza parametro**: Mostra coordinate, costellazione, zodiaco.

**Output (info)**:
```
[nome] in [costellazione] [zodiaco]
  Dec=[declinazione] AR=[ascensione_retta] R=[distanza]
```

---

### `factor`
**Sintassi**: `factor <fattore> <nome_corpo>`

**Descrizione**: Modifica il fattore di scala di un corpo celeste.

**Parametri**:
- `fattore`: Moltiplicatore dimensioni (es. 10 = 10 volte più grande)
- `nome_corpo`: Nome del corpo da scalare

**Risultati**: Rimuove e ricrea il corpo con nuove dimensioni.

---

## Comandi Camera

### `near`
**Sintassi**: `near <distanza>`

**Descrizione**: Imposta il piano near della camera.

**Parametri**:
- `distanza`: Distanza minima di rendering

---

### `far`
**Sintassi**: `far <distanza>`

**Descrizione**: Imposta il piano far della camera.

**Parametri**:
- `distanza`: Distanza massima di rendering

---

### `up`
**Sintassi**: `up <oggetto> <x> <y> <z>`

**Descrizione**: Imposta il vettore "up" per camera o oggetto.

**Parametri**:
- `oggetto`: `camera` o nome di un corpo celeste
- `x`, `y`, `z`: Componenti del vettore up

---

### `lookAt`
**Sintassi**: `lookAt <oggetto> <x> <y> <z>`

**Descrizione**: Imposta il punto target per un oggetto.

**Parametri**:
- `oggetto`: Nome dell'oggetto
- `x`, `y`, `z`: Coordinate del punto target

---

### `height`
**Sintassi**: `height <gradi>`

**Descrizione**: Imposta l'altezza della vista sull'orizzonte.

**Parametri**:
- `gradi`: Altezza in gradi (-90 a 90)

---

### `azimut`
**Sintassi**: `azimut <gradi>`

**Descrizione**: Imposta l'azimut della vista.

**Parametri**:
- `gradi`: Azimut in gradi (0-360)

---

### `coordinate`
**Sintassi**: `coordinate <longitudine> <latitudine>`

**Descrizione**: Imposta le coordinate dell'osservatore.

**Parametri**:
- `longitudine`: Longitudine in gradi
- `latitudine`: Latitudine in gradi

---

## Comandi Rendering

### `render`
**Sintassi**: `render`

**Descrizione**: Esegue un ciclo di rendering completo.

**Risultati**: Genera XML completo con tutte le posizioni aggiornate degli oggetti celesti.

---

### `light`
**Sintassi**: `light <true|false|on|off>`

**Descrizione**: Attiva/disattiva la luce ambientale.

**Parametri**:
- `true`/`on`: Attiva luce ambientale
- `false`/`off`: Disattiva luce ambientale

**Output**:
```xml
<results>
  <renderer>
    <light name="ambient" type="ambient" color="0x323232" />
    <!-- o con <hidden /> -->
  </renderer>
  <terminal>Ambient light set to on/off</terminal>
</results>
```

---

### `scale`
**Sintassi**: `scale <valore>`

**Descrizione**: Imposta il fattore di scala globale.

**Parametri**:
- `valore`: Fattore di scala

---

### `default`
**Sintassi**: `default`

**Descrizione**: Ripristina tutti i valori predefiniti.

**Risultati**: Reset di FOV, zoom, simulazione a valori iniziali.

---

## Comando Generico

### `command`
**Sintassi**: Qualsiasi comando non riconosciuto viene trattato come comando generico.

**Descrizione**: Se invocato senza parametri o con nome corpo celeste sconosciuto, mostra informazioni generali:

**Output**:
```
UT [data_ut]
LT [data_locale] (add DST if needed)
DT [data_dt]
JD [julian_day]
look from [posizione]
  [città_vicina]
  [informazioni_timezone]
  x=... y=... z=...
look to [target]
  x=... y=... z=...
```

---

## Note Importanti

### Integrazione GeoNames
Molti comandi usano l'API di GeoNames (username: `baldoweb`):
- Ricerca città per coordinate
- Informazioni timezone
- Calcolo alba/tramonto
- Offset DST

### Sistema di Coordinate
- **Equatoriali**: AR (Ascensione Retta) in ore, Dec (Declinazione) in gradi
- **Eclittiche**: Longitudine (L/λ), Latitudine (B/β) in gradi
- **Orizzontali**: Altezza e Azimut in gradi
- **Cartesiane**: x, y, z in AU (Unità Astronomiche)

### Formati Data/Ora
- **UT**: Tempo Universale
- **DT**: Tempo Dinamico (include correzioni ΔT)
- **LT**: Tempo Locale (UT + offset timezone)
- **JD**: Giorno Giuliano

### Formato Output
La maggior parte dei comandi restituisce XML strutturato:
```xml
<results>
  <terminal>Output testuale per console</terminal>
  <dialog>HTML per dialoghi UI</dialog>
  <data>Aggiornamenti campi form</data>
  <renderer>Comandi di rendering 3D</renderer>
</results>
```

---

## Esempi Completi

### Osservare Roma al tramonto
```
date 21-06-2024 19:30:00 UT
observe 12.4964 41.9028 W
sun_rise
```

### Trovare Orione
```
constellation ORI
asterism ORI
sky 5.5 0
```

### Animare il sistema solare
```
simulation 3600
execute_loop on
lookat sun
orbitControl 1
```

### Vista dalla Luna
```
lookfrom moon 0 0 earth
zoom 5
```
