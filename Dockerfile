# --- STAGE 1: BUILDER ---
FROM node:18-bookworm AS builder

# Installazione dipendenze di sistema
RUN apt-get update && apt-get install -y \
    g++ make python3 libmariadb-dev libcurl4-openssl-dev nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# 1. Copia SOLO le sorgenti C++ per compilare le librerie
COPY cpp ./cpp

# Compilazione librerie C++
RUN cd cpp/libXML && make lib && ldconfig
RUN cd cpp/libJSON && make lib && ldconfig
RUN cd cpp/libMYSQL && make && ldconfig
RUN cd cpp/libSystem && make lib && ldconfig

ENV COMMON_INCLUDES="-I/app/cpp/libPlanetarium -I/app/cpp/libPlanetarium/VSOP/include -I/app/cpp/libPlanetarium/timer/include -I/app/cpp/libPlanetarium/date/include -I/app/cpp/libPlanetarium/color -I/app/cpp/libPlanetarium/bitmap -I/app/cpp/libPlanetarium/webClient/include -I/app/cpp/geometria/include -I/app/cpp/CMat -I/app/cpp/CMat/interprete/include -I/app/cpp/libSystem -I/app/cpp/Utils -I/app/cpp/libMYSQL -I/app/cpp/libXML/include -I/app/cpp/libJSON/include -I/usr/include/mysql -I/usr/include/mariadb"

# Compilazione moduli specifici
RUN cd cpp/Utils && g++ -fPIC -Wall -Wextra -std=c++17 -I. $COMMON_INCLUDES -c Utils.cpp -o Utils.o
RUN cd cpp/libPlanetarium/elp82b && g++ -fPIC -Wall -Wextra -std=c++17 -I. $COMMON_INCLUDES -c calc_interpolated_elements.cpp -o calc_interpolated_elements.o
RUN cd cpp/geometria/source && g++ -fPIC -Wall -Wextra -std=c++17 -I. $COMMON_INCLUDES -c horizont.cpp -o horizont.o
RUN cd cpp/libPlanetarium/color && g++ -fPIC -Wall -Wextra -std=c++17 -I. $COMMON_INCLUDES -c color.cpp -o color.o
RUN cd cpp/libPlanetarium/timer/source && g++ -fPIC -Wall -Wextra -std=c++17 -I. $COMMON_INCLUDES -c timer.cpp -o timer.o

# ... [Tutta la parte C++ precedente rimane IDENTICA] ...

# Creazione libreria finale Planetarium
RUN cd cpp/libPlanetarium && make -f Makefile clean && cp /app/cpp/Utils/Utils.o . && cp /app/cpp/libPlanetarium/elp82b/*.o . \
    && cp /app/cpp/geometria/source/*.o . && cp /app/cpp/libPlanetarium/color/*.o . && cp /app/cpp/libPlanetarium/timer/source/*.o . \
    && make -f Makefile libPlanetarium.so && make -f Makefile install

# -----------------------------------------------------------------------------
# 2. Ottimizzazione Cache per Node.js
# -----------------------------------------------------------------------------

# A. Copia SOLO i file di configurazione e i sorgenti C++ dei binding
# Questo crea un layer che cambia SOLO se modifichi le dipendenze o il binding nativo
COPY server/package*.json ./server/
COPY server/native ./server/native

# B. Esegui le compilazioni pesanti
RUN cd server/native && npm install -g node-gyp && node-gyp rebuild --verbose
RUN cd server && npm install --omit=dev

# C. ORA copia il resto del codice Node.js (i file .js, le rotte, ecc.)
# Modificare un file .js invaliderà solo questo layer, saltando la compilazione sopra!
COPY server ./server

# --- STAGE 2: RUNTIME ---

FROM node:18-bookworm-slim

RUN apt-get update && apt-get install -y \
    libmariadb3 libcurl4 \
    && rm -rf /var/lib/apt/lists/* \
    && npm install -g pm2 \
    && pm2 install pm2-logrotate \
    && pm2 set pm2-logrotate:max_size 10M \
    && pm2 set pm2-logrotate:retain 5

WORKDIR /app

# Copia librerie native dallo stage builder
COPY --from=builder /usr/lib/libXML.so /usr/lib/
COPY --from=builder /usr/lib/libJSON.so /usr/lib/
COPY --from=builder /usr/lib/libMYSQL.so /usr/lib/
COPY --from=builder /usr/lib/libsystem.so /usr/lib/
COPY --from=builder /usr/lib/libPlanetarium.so /usr/lib/
RUN ldconfig

# Copia il server Node compilato
COPY --from=builder /app/server /app/server
RUN mkdir -p /app/server/logs
RUN mkdir -p /app/data/uploads && chmod 777 /app/data/uploads

EXPOSE 3000

WORKDIR /app/server
# Avvio con pm2-runtime
CMD ["pm2-runtime", "start", "ecosystem.config.cjs"]
