/**
 * planetarium-controls.js
 * Gestione input: tastiera e mouse con raycasting
 *
 * Controlli INIBITI quando:
 *  - focus sulla casella comandi  (commandFocused = true)
 *  - controllo orbita attivo      (orbitEnabled   = true)
 *
 * Nota: mousedown sul canvas, ma mousemove/mouseup su document
 * così il drag non si perde se il mouse esce dal canvas.
 */

const controls = {

    orbitEnabled:   false,
    commandFocused: false,
    _horizont:      true,   // stato corrente dell'orizzonte
    _ambientLight:  true,   // stato corrente luce ambiente

    _dragging:  false,
    _prevMouse: { x: 0, y: 0 },

    // ── Init ─────────────────────────────────────────────────────────────────

    init: function(canvas, commandInput) {
        // Tastiera — su document
        document.addEventListener('keydown', (e) => this._onKeyDown(e));
        document.addEventListener('keyup',   (e) => this._onKeyUp(e));

        // Focus casella comandi
        commandInput.addEventListener('focus', () => { this.commandFocused = true;  });
        commandInput.addEventListener('blur',  () => { this.commandFocused = false; });

        // Mouse: DOWN sul canvas, MOVE/UP su document
        // (così il drag continua anche fuori dal canvas)
        canvas.addEventListener('mousedown',   (e) => this._onMouseDown(e));
        document.addEventListener('mousemove', (e) => this._onMouseMove(e));
        document.addEventListener('mouseup',   (e) => this._onMouseUp(e));

        canvas.addEventListener('dblclick',    (e) => this._onMouseDblClick(e));
        canvas.addEventListener('contextmenu', (e) => e.preventDefault());
        canvas.addEventListener('wheel',       (e) => this._onWheel(e), { passive: false });

        logToPage('🎮 Controlli inizializzati', 'success');
        logToPage('⌨️  Frecce: muovi | +/-: zoom | N/S/E/W/Z: direzione | H: orizzonte | C: console | R: reset', 'info');
    },

    // ── Helpers ───────────────────────────────────────────────────────────────

    // Tastiera + mouse inibiti quando orbita è attiva OPPURE focus su input
    _navBlocked: function() {
        return this.orbitEnabled || this.commandFocused;
    },

    _send: function(cmd) { sendCommand(cmd); },

    _isOnCanvas: function(event) {
        return event.target === webgl.renderer.domElement;
    },

    _ndc: function(event) {
        const rect = webgl.renderer.domElement.getBoundingClientRect();
        return new THREE.Vector2(
            ((event.clientX - rect.left) / rect.width)  *  2 - 1,
            ((event.clientY - rect.top)  / rect.height) * -2 + 1
        );
    },

    _raycast: function(event) {
        const raycaster = new THREE.Raycaster();
        raycaster.setFromCamera(this._ndc(event), webgl.camera);
        
        // FIX: Usa webgl.click_objects se disponibile (come vecchio script)
        // altrimenti fallback a tutti gli oggetti con nome
        let targets;
        if (webgl.click_objects && webgl.click_objects.length > 0) {
            // ← FIX: Filtra solo oggetti visibili (skip hidden objects)
            targets = webgl.click_objects.filter(obj => obj.visible);
        } else {
            targets = [];
            webgl.scene.traverse((o) => {
                // ← FIX: Controlla anche visibilità
                if ((o.isMesh || o.isPoints || o.isLine) && o.name && o.visible) {
                    // Escludi linee di griglia/guida che non dovrebbero essere clickabili
                    const excludeNames = ['eclittica', 'ecliptic', 'equatore', 'equator', 
                                         'hor_circle', 'hor_line', 'hor_nord', 'grid', 'axis'];
                    if (!excludeNames.includes(o.name)) {
                        targets.push(o);
                    }
                }
            });
        }
        
        const hits = raycaster.intersectObjects(targets, true);
        
        if (hits.length === 0) return null;
        
        // PRIORITÀ: Se ci sono Mesh tra i risultati, preferiscile alle Line
        // (il sole è una Mesh, l'eclittica è una Line)
        // ← FIX: Assicurati anche che il mesh hit sia visibile
        const meshHit = hits.find(h => h.object.isMesh && h.object.visible);
        return meshHit || (hits[0].object.visible ? hits[0] : null);
    },

    _sendMouseMsg: function(action, event, hit, type) {
        if (!wsIsOpen()) return;
        const btn = event.button || 0;
        const p   = hit.point;
        
        // Chiudi dialog esistente su mousedown, salva evento per nuovo dialog su mouseup
        if (action === 'mouse' && type === 'down') {
            if (typeof closeDialog === 'function') {
                closeDialog();  // Chiude vecchio dialog se la funzione esiste
            }
            window.currentDialogEvent = event;  // Salva per nuovo dialog
        }
        
        wsSend({
            pagina: "PLANETARIUM",
            doc:    "command",
            pageX:  event.pageX,
            pageY:  event.pageY,
            data:   `${action} ${btn} ${hit.object.name} ${p.x} ${p.y} ${p.z} ${hit.distance} ${type}`
        });
        
        // Log solo eventi significativi (down e dblclick), non move e up
        if (type === 'down' || action === 'mouse_dblclick') {
            logToPage(`🖱️ ${action}: ${hit.object.name}`, 'info');
        }
    },

    // ── Tastiera ─────────────────────────────────────────────────────────────

    _onKeyDown: function(e) {
        if (this._navBlocked()) return;   // ← inibizione unica

        switch (e.which) {
            case 38: e.preventDefault(); this._send('move 1 0');  logToPage('⬆️ alt +1°',  'info'); break;
            case 40: e.preventDefault(); this._send('move -1 0'); logToPage('⬇️ alt -1°',  'info'); break;
            case 39: e.preventDefault(); this._send('move 0 1');  logToPage('➡️ az +1°',   'info'); break;
            case 37: e.preventDefault(); this._send('move 0 -1'); logToPage('⬅️ az -1°',   'info'); break;

            case 187: case 171: case 61:
                e.preventDefault(); this._send('zoom +'); logToPage('🔍 zoom+', 'info'); break;
            case 189: case 173:
                e.preventDefault(); this._send('zoom -'); logToPage('🔎 zoom-', 'info'); break;

            case 78: e.preventDefault(); this._send('view N'); logToPage('🧭 Nord',  'info'); break;
            case 83: e.preventDefault(); this._send('view S'); logToPage('🧭 Sud',   'info'); break;
            case 69: e.preventDefault(); this._send('view E'); logToPage('🧭 Est',   'info'); break;
            case 87: e.preventDefault(); this._send('view W'); logToPage('🧭 Ovest', 'info'); break;
            case 79: e.preventDefault(); this._send('view W'); logToPage('🧭 Ovest', 'info'); break;
            case 90: e.preventDefault(); this._send('view Z'); logToPage('🧭 Zenit', 'info'); break;

            case 72: e.preventDefault();
                this._horizont = !this._horizont;
                this._send(this._horizont ? 'horizont true' : 'horizont false');
                logToPage(`🌅 orizzonte: ${this._horizont ? 'on' : 'off'}`, 'info');
                break;
            case 76: e.preventDefault();
                this._ambientLight = !this._ambientLight;
                this._send(this._ambientLight ? 'light on' : 'light off');
                logToPage(`💡 luce ambiente: ${this._ambientLight ? 'on' : 'off'}`, 'info');
                break;
            case 67: e.preventDefault(); toggleConsole();              break;
            case 82: e.preventDefault(); this._send('default');       logToPage('🔄 reset',    'info'); break;
        }
    },

    _onKeyUp: function(e) { /* placeholder */ },

    // ── Mouse ─────────────────────────────────────────────────────────────────

    _onMouseDown: function(e) {
        if (this.orbitEnabled) {
            // Solo drag orbita, nessun comando al server
            this._dragging  = true;
            this._prevMouse = { x: e.clientX, y: e.clientY };
            return;
        }
        if (this.commandFocused) return;

        e.preventDefault();
        const hit = this._raycast(e);
        if (hit) {
            this._dragging = true;  // ← FIX: imposta dragging per permettere mouseup
            this._sendMouseMsg('mouse', e, hit, 'down');
        }
    },

    _onMouseMove: function(e) {
        if (this.orbitEnabled) {
            if (!this._dragging) return;
            // Orbit drag — ruota camera localmente, non invia al server
            const dx = e.clientX - this._prevMouse.x;
            const dy = e.clientY - this._prevMouse.y;
            const speed = 0.005;
            webgl.camera.position.applyAxisAngle(new THREE.Vector3(0, 1, 0), -dx * speed);
            webgl.camera.position.applyAxisAngle(new THREE.Vector3(1, 0, 0), -dy * speed);
            webgl.camera.lookAt(0, 0, 0);
            this._prevMouse = { x: e.clientX, y: e.clientY };
            return;
        }
        // ← FIX: Non inviare mouse_move al server - solo orbit locale
        // Il server non ha bisogno di sapere ogni singolo movimento del mouse
    },

    _onMouseUp: function(e) {
        if (this.orbitEnabled) {
            this._dragging = false;
            return;
        }
        if (this.commandFocused) { 
            this._dragging = false; 
            return; 
        }

        // ← FIX: Non inviare mouse_up al server
        // Il server riceve già mousedown che è sufficiente per identificare il click
        // mouseup causava doppi messaggi e doppi dialog
        this._dragging = false;
    },

    _onMouseDblClick: function(e) {
        if (this._navBlocked()) return;   // ← inibizione unica
        e.preventDefault();
        const hit = this._raycast(e);
        if (hit) this._sendMouseMsg('mouse_dblclick', e, hit, 'dblclick');
    },

    _onWheel: function(e) {
        e.preventDefault();
        if (this.commandFocused) return;

        if (this.orbitEnabled) {
            // Zoom fisico della camera
            const factor = e.deltaY > 0 ? 1.1 : 0.9;
            webgl.camera.position.multiplyScalar(factor);
        } else {
            this._send(e.deltaY > 0 ? 'zoom -' : 'zoom +');
        }
    },

    // ── Toggle orbit ─────────────────────────────────────────────────────────

    setOrbit: function(enabled) {
        this.orbitEnabled = enabled;
        this._dragging    = false;   // reset drag state al cambio modalità
        logToPage(`🎮 Orbita: ${enabled ? 'ON (kbd/mouse inibiti)' : 'OFF'}`, 'info');
    }
};
