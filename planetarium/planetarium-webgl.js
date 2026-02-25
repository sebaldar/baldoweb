/**
 * planetarium-webgl.js
 * Gestione scena Three.js: renderer, camera, luci, oggetti, primitives
 */
const webgl = {
    scene:         null,
    camera:        null,
    renderer:      null,
    sunLight:      null,
    click_objects: [],  // ← Array di oggetti clickabili
    
    // ← DEBUG: Flag per tracciare visibilità
    _debugVisibility: false,
    _visibilityStates: new Map(),  // Traccia stato visibilità per ogni oggetto

    // ── Init ─────────────────────────────────────────────────────────────────
    init: function(container) {
        this.scene = new THREE.Scene();
        this.camera = new THREE.PerspectiveCamera(
            75, window.innerWidth / window.innerHeight, 0.00001, 1000
        );
        try {
            this.renderer = new THREE.WebGLRenderer({ antialias: true, powerPreference: "high-performance" });
            this.renderer.setSize(window.innerWidth, window.innerHeight);
            this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
            this.renderer.shadowMap.enabled = true;
            this.renderer.shadowMap.type    = THREE.PCFSoftShadowMap;
            this.renderer.setClearColor(0x000000, 1);
            container.appendChild(this.renderer.domElement);
        } catch (e) {
            container.innerHTML = '<div style="color:white;padding:50px;text-align:center"><h1>⚠️ Errore WebGL</h1></div>';
            return false;
        }
        // ← FIX: Luce solare come PointLight al centro del sistema (sole)
        // Non DirectionalLight perché vogliamo illuminazione radiale realistica
        this.sunLight = new THREE.PointLight(0xffffff, 2.0, 0, 0); // decay=0 nessuna attenuazione
        this.sunLight.name = 'sunlight';
        this.sunLight.castShadow = false;
        this.sunLight.position.set(0, 0, 0); // Sarà aggiornato alla posizione del sole
        this.scene.add(this.sunLight);
        console.log('☀️ Sunlight inizializzata: PointLight con intensità 2.0');
        
        this.createSkySphere();
        window.addEventListener('resize', () => this.onWindowResize());
        this.animate();
        logToPage('🚀 WebGL inizializzato', 'success');
        return true;
    },

    createSkySphere: function() {
        const tex = new THREE.TextureLoader().load(
            '/public/default/rif_TychoSkymapII.t5_04096x02048.jpg',
            () => logToPage('🌌 Texture firmamento caricata', 'success'),
            undefined,
            () => logToPage('❌ Errore caricamento firmamento', 'error')
        );
        const mesh = new THREE.Mesh(
            new THREE.SphereGeometry(20, 64, 32),
            new THREE.MeshBasicMaterial({ map: tex, side: THREE.BackSide })
        );
        mesh.name = 'sky';
        mesh.rotation.x = Math.PI / 2;
        this.scene.add(mesh);
        this.click_objects.push(mesh);
    },

    // ── Processo XML dal server ───────────────────────────────────────────────
    // Nota: usiamo getElementsByTagName invece di querySelector perché
    // querySelector non è affidabile su documenti XML in tutti i browser
    processRenderer: function(rendererEl) {
        // <data> è fratello di <renderer> nel documento XML del server
        this.updateGeneralInfo(rendererEl.ownerDocument);
        const camList = rendererEl.getElementsByTagName('camera');
        if (camList.length > 0) this.processCamera(camList[0]);
        const lightList = rendererEl.getElementsByTagName('light');
        for (let i = 0; i < lightList.length; i++) this.processLight(lightList[i]);
        const primList = rendererEl.getElementsByTagName('primitive');
        let visible = 0;
        for (let i = 0; i < primList.length; i++) {
            this.processPrimitive(primList[i]);
            const name = primList[i].getAttribute('name');
            const obj  = this.scene.getObjectByName(name);
            if (obj && obj.visible) visible++;
        }
        const cnt = document.getElementById('object-count');
        if (cnt) cnt.textContent = visible;
    },

    // ── Info panel ───────────────────────────────────────────────────────────
    updateGeneralInfo: function(xmlDoc) {
        const trySet = (domId, xmlSel, attr) => {
            try {
                const node = xmlDoc.querySelector(xmlSel);
                if (node) {
                    const val = node.getAttribute(attr);
                    const el  = document.getElementById(domId);
                    if (el && val) el.textContent = val;
                }
            } catch(e) { /* ignora errori di selettore */ }
        };
        
        // Parsa data/ora e popola input fields solo se NON bloccati
        try {
            const isLocked = window.dateTimeInputsLocked && window.dateTimeInputsLocked();
            
            if (!isLocked) {
                const dateNode = xmlDoc.querySelector('[id="the_date"]');
                if (dateNode) {
                    const dateTimeStr = dateNode.getAttribute('value');
                    if (dateTimeStr) {
                        const parts = dateTimeStr.trim().split(/\s+/);
                        
                        if (parts.length >= 2) {
                            let datePart = parts[0];
                            let timePart = parts[1];
                            
                            // Normalizza formato ora (aggiungi zeri iniziali)
                            const timeParts = timePart.split(':');
                            if (timeParts.length >= 2) {
                                const hours = timeParts[0].padStart(2, '0');
                                const minutes = timeParts[1].padStart(2, '0');
                                const seconds = timeParts[2] ? timeParts[2].padStart(2, '0') : '00';
                                timePart = `${hours}:${minutes}:${seconds}`;
                            }
                            
                            // Converti data in formato yyyy-mm-dd
                            const dateParts = datePart.split('-');
                            if (dateParts.length === 3) {
                                const num1 = parseInt(dateParts[0]);
                                const num3 = parseInt(dateParts[2]);
                                
                                if (num1 > 31) {
                                    // Già yyyy-mm-dd
                                    datePart = `${dateParts[0]}-${dateParts[1].padStart(2,'0')}-${dateParts[2].padStart(2,'0')}`;
                                } else if (num3 > 31) {
                                    // È dd-mm-yyyy, converti
                                    datePart = `${dateParts[2]}-${dateParts[1].padStart(2,'0')}-${dateParts[0].padStart(2,'0')}`;
                                } else {
                                    // Ambiguo, assumiamo dd-mm-yyyy se giorno <= 12
                                    if (num1 <= 12) {
                                        datePart = `${dateParts[2]}-${dateParts[1].padStart(2,'0')}-${dateParts[0].padStart(2,'0')}`;
                                    } else {
                                        datePart = `${dateParts[0]}-${dateParts[1].padStart(2,'0')}-${dateParts[2].padStart(2,'0')}`;
                                    }
                                }
                            }
                            
                            // Popola input
                            const dateInput = document.getElementById('sim-date');
                            if (dateInput && dateInput !== document.activeElement) {
                                dateInput.value = datePart;
                            }
                            
                            const timeInput = document.getElementById('sim-time');
                            if (timeInput && timeInput !== document.activeElement) {
                                timeInput.value = timePart;
                            }
                        }
                    }
                }
            }
        } catch(e) { 
            console.error('❌ Error parsing date/time:', e); 
        }
        
        trySet('julian-day', '[id="the_jd"]',      'value');
        trySet('latitude',   '[id="latitudine"]',  'value');
        trySet('longitude',  '[id="longitudine"]', 'value');
        trySet('azimut',     '[id="azimut"]',      'value');
        trySet('height',     '[id="height"]',      'value');
    },

    // ── Camera ───────────────────────────────────────────────────────────────
    processCamera: function(camEl) {
        const first = (tag) => {
            const l = camEl.getElementsByTagName(tag);
            return l.length > 0 ? l[0] : null;
        };
        const posEl = first('position');
        if (posEl) {
            const x = parseFloat(posEl.getAttribute('x'));
            const y = parseFloat(posEl.getAttribute('y'));
            const z = parseFloat(posEl.getAttribute('z'));
            if (isFinite(x) && isFinite(y) && isFinite(z)) {
                this.camera.position.set(x, y, z);
                logToPage(`📷 pos (${x.toExponential(2)}, ${y.toExponential(2)}, ${z.toExponential(2)})`, 'info');
            }
        }
        const upEl = first('up');
        if (upEl) {
            const x = parseFloat(upEl.getAttribute('x'));
            const y = parseFloat(upEl.getAttribute('y'));
            const z = parseFloat(upEl.getAttribute('z'));
            if (isFinite(x) && isFinite(y) && isFinite(z))
                this.camera.up.set(x, y, z).normalize();
        }
        const latEl = first('lookat');
        if (latEl) {
            const x = parseFloat(latEl.getAttribute('x'));
            const y = parseFloat(latEl.getAttribute('y'));
            const z = parseFloat(latEl.getAttribute('z'));
            if (isFinite(x) && isFinite(y) && isFinite(z)) {
                this.camera.lookAt(x, y, z);
                logToPage(`📷 lookAt (${x.toExponential(2)}, ${y.toExponential(2)}, ${z.toExponential(2)})`, 'info');
            }
        }
        const apply = (tag, setter) => {
            const el = first(tag);
            if (el) {
                const v = parseFloat(el.getAttribute('value'));
                if (isFinite(v) && v > 0) { setter(v); this.camera.updateProjectionMatrix(); }
            }
        };
        apply('fov',  v => { this.camera.fov  = v; });
        apply('zoom', v => { this.camera.zoom = v; });
        apply('near', v => { this.camera.near = v; });
        apply('far',  v => { this.camera.far  = v; });
    },

    // ── Luci ─────────────────────────────────────────────────────────────────
    processLight: function(lightEl) {
        const name      = lightEl.getAttribute('name')      || '';
        const type      = lightEl.getAttribute('type')      || 'point';
        const color     = parseInt(lightEl.getAttribute('color') || '0xffffff', 16);
        const intensity = parseFloat(lightEl.getAttribute('intensity') || '1.0');
        const hidden    = lightEl.getElementsByTagName('hidden').length > 0;
        const posList   = lightEl.getElementsByTagName('position');
        const hasPosEl  = posList.length > 0;
        let px = 0, py = 0, pz = 0;
        if (hasPosEl) {
            px = parseFloat(posList[0].getAttribute('x'));
            py = parseFloat(posList[0].getAttribute('y'));
            pz = parseFloat(posList[0].getAttribute('z'));
        }
        // La luce solare esiste già dalla init(): aggiornala direttamente
        if (name === 'sunlight') {
            this.sunLight.color.setHex(color);
            // ← FIX: Intensità alta per PointLight (decade con distanza al quadrato)
            this.sunLight.intensity = Math.max(intensity * 2.0, 2.0);
            this.sunLight.visible   = !hidden;
            if (hasPosEl && isFinite(px) && isFinite(py) && isFinite(pz)) {
                // ← CRITICAL: Posiziona la luce ESATTAMENTE dove si trova il sole
                this.sunLight.position.set(px, py, pz);
                // Log solo cambio significativo (non ogni frame)
                if (!this._lastSunPos || 
                    Math.abs(this._lastSunPos.x - px) > 0.1 ||
                    Math.abs(this._lastSunPos.y - py) > 0.1 ||
                    Math.abs(this._lastSunPos.z - pz) > 0.1) {
                    console.log(`☀️ Sunlight (PointLight): pos(${px.toFixed(3)}, ${py.toFixed(3)}, ${pz.toFixed(3)}) int=${this.sunLight.intensity.toFixed(2)}`);
                    this._lastSunPos = {x: px, y: py, z: pz};
                }
            }
            return;
        }
        // Altre luci
        let light = this.scene.getObjectByName(name, true);
        if (!light) {
            // ← FIX: Luce ambiente minima 0.4 per vedere pianeti anche in ombra parziale
            const finalIntensity = (type === 'ambient') ? Math.max(intensity, 0.4) : intensity;
            switch (type) {
                case 'ambient':     light = new THREE.AmbientLight(color, finalIntensity);      break;
                case 'directional': light = new THREE.DirectionalLight(color, finalIntensity);  break;
                default:            light = new THREE.PointLight(color, finalIntensity, 0, 0); break;
            }
            light.name = name;
            if (lightEl.getElementsByTagName('shadow').length > 0 && type !== 'ambient') {
                light.castShadow = true;
                light.shadow.mapSize.set(2048, 2048);
            }
            this.scene.add(light);
            if (type === 'ambient') {
                console.log(`💡 Ambient light: int=${finalIntensity.toFixed(2)} (min 0.4)`);
            }
        }
        light.color.setHex(color);
        // ← FIX: Applica minimo anche agli aggiornamenti
        light.intensity = (type === 'ambient') ? Math.max(intensity, 0.4) : intensity;
        light.visible   = !hidden;
        if (hasPosEl && isFinite(px) && isFinite(py) && isFinite(pz) && type !== 'ambient')
            light.position.set(px, py, pz);
    },

    // ── Primitives ───────────────────────────────────────────────────────────
    processPrimitive: function(primEl) {
        const name = primEl.getAttribute('name') || '';
        const type = primEl.getAttribute('type') || 'sphere';
        let obj = this.scene.getObjectByName(name, true);
        
        // Gestione ricreazione oggetto
        if (obj && primEl.getElementsByTagName('recreate').length > 0) {
            this.scene.remove(obj);
            if (obj.geometry) obj.geometry.dispose();
            const idx = this.click_objects.indexOf(obj);
            if (idx > -1) this.click_objects.splice(idx, 1);
            obj = null;
        }
        
        // Gestione rimozione oggetto
        if (obj && primEl.getElementsByTagName('remove').length > 0) {
            this.scene.remove(obj);
            if (obj.geometry) obj.geometry.dispose();
            const idx = this.click_objects.indexOf(obj);
            if (idx > -1) this.click_objects.splice(idx, 1);
            return;
        }
        
        // Creazione nuovo oggetto se non esiste
        if (!obj) {
            // ← FIX: Sky è gestita da createSkySphere(), non crearla qui
            // MA se esiste già, recuperala per permettere aggiornamenti
            if (name === 'sky') {
                obj = this.scene.getObjectByName('sky', true);
                if (!obj) {
                    console.warn('⚠️ Sky primitive received but sky sphere not yet created');
                    return;
                }
                console.log('🌌 Sky found, applying updates');
            } else {
                obj = this.createPrimitiveObject(primEl, type, name);
                if (!obj) return;
                obj.name = name;
                
                const shadowList = primEl.getElementsByTagName('shadow');
                if (shadowList.length > 0) {
                    const mode = shadowList[0].getAttribute('mode') || '';
                    obj.castShadow    = (mode === 'cast'    || mode === 'both');
                    obj.receiveShadow = (mode === 'receive' || mode === 'both');
                }
                
                obj.visible = primEl.getElementsByTagName('hidden').length === 0;
                
                if (primEl.getElementsByTagName('clickable').length > 0) {
                    if (!this.click_objects.includes(obj)) {
                        this.click_objects.push(obj);
                    }
                }
                
                this.scene.add(obj);
                this._visibilityStates.set(name, obj.visible);
            }
        }
        
        // ← IMPORTANTE: updateObjectParameters ora viene chiamato anche per sky
        this.updateObjectParameters(obj, primEl);
        
        // ← CRITICAL DEBUG: Visibilità con logging dettagliato
        const hasHidden = primEl.getElementsByTagName('hidden').length > 0;
        const hasShow = primEl.getElementsByTagName('show').length > 0;
        const prevVisible = this._visibilityStates.get(name);
        
        if (this._debugVisibility && (hasHidden || hasShow)) {
            console.log(`🔍 VISIBILITY CHECK: ${name}`, {
                hasHiddenTag: hasHidden,
                hasShowTag: hasShow,
                currentVisible: obj.visible,
                previousVisible: prevVisible
            });
        }
        
        if (hasHidden) {
            if (obj.visible) {
                obj.visible = false;
                this._visibilityStates.set(name, false);
                console.log(`❌ HIDDEN: ${name} → visible false`);
                
                // ← FIX: Rimuovi da click_objects quando nascosto
                const idx = this.click_objects.indexOf(obj);
                if (idx > -1) {
                    this.click_objects.splice(idx, 1);
                    console.log(`🖱️ ${name} non più clickable (hidden)`);
                }
            }
        } else if (hasShow) {
            if (!obj.visible) {
                obj.visible = true;
                this._visibilityStates.set(name, true);
                console.log(`✅ SHOW: ${name} → visible true`);
                
                // ← FIX: Aggiungi a click_objects se ha tag clickable e viene mostrato
                if (primEl.getElementsByTagName('clickable').length > 0) {
                    if (!this.click_objects.includes(obj)) {
                        this.click_objects.push(obj);
                        console.log(`🖱️ ${name} di nuovo clickable (show)`);
                    }
                }
            }
        } else {
            // NO tag hidden/show → mantieni stato, ma traccia se cambia
            if (this._debugVisibility && prevVisible !== undefined && prevVisible !== obj.visible) {
                console.log(`⚠️ VISIBILITY CHANGED WITHOUT TAG: ${name} ${prevVisible} → ${obj.visible}`);
            }
            this._visibilityStates.set(name, obj.visible);
        }
    },

    createPrimitiveObject: function(primEl, type, name) {
        let geometry, material;
        const matList = primEl.getElementsByTagName('material');
        const matEl   = matList.length > 0 ? matList[0] : null;
        if (type === 'sphere') {
            const r  = parseFloat(primEl.getAttribute('radius'))         || 1.0;
            const ws = parseFloat(primEl.getAttribute('widthSegments'))  || 32;
            const hs = parseFloat(primEl.getAttribute('heightSegments')) || 32;
            geometry = new THREE.SphereGeometry(r, ws, hs);
        } else {
            const verts    = [];
            const vertList = primEl.getElementsByTagName('vertice');
            for (let i = 0; i < vertList.length; i++) {
                const parts = vertList[i].textContent.trim().split(/[\s,]+/);
                if (parts.length >= 3)
                    verts.push(parseFloat(parts[0]), parseFloat(parts[1]), parseFloat(parts[2]));
            }
            if (verts.length === 0) return null;
            geometry = new THREE.BufferGeometry();
            geometry.setAttribute('position', new THREE.Float32BufferAttribute(verts, 3));
        }
        material = this.createMaterial(matEl, type);
        // Texture per sfere: MeshPhongMaterial (risponde meglio alla luce)
        // tranne il sole che è autoilluminante → MeshBasicMaterial
        if (type === 'sphere' && matEl) {
            const mapPath = matEl.getAttribute('map');
            if (mapPath) {
                const url = mapPath.replace('/my_image/', '/public/');
                const tex = new THREE.TextureLoader().load(
                    url,
                    () => {}, // Silent success
                    undefined,
                    () => logToPage(`⚠️ Texture ${name} KO: ${url}`, 'warning')
                );
                if (name === 'sun') {
                    material = new THREE.MeshBasicMaterial({ map: tex });
                } else {
                    // ← FIX: MeshPhongMaterial standard senza emissive
                    // La PointLight sul sole fornisce illuminazione corretta
                    material = new THREE.MeshPhongMaterial({ 
                        map: tex, 
                        shininess: 8
                    });
                }
            } else if (name !== 'sun') {
                const color = matEl ? parseInt(matEl.getAttribute('color') || '0xffffff', 16) : 0xffffff;
                material = new THREE.MeshPhongMaterial({ 
                    color, 
                    shininess: 8
                });
            }
        }
        switch (type) {
            case 'line':  return new THREE.Line(geometry, material);
            case 'point': return new THREE.Points(geometry, material);
            default:      return new THREE.Mesh(geometry, material);
        }
    },

    createMaterial: function(matEl, type) {
        if (!matEl) {
            if (type === 'line')  return new THREE.LineBasicMaterial({ color: 0xffffff });
            if (type === 'point') return new THREE.PointsMaterial({ color: 0xffffff, size: 2, sizeAttenuation: false });
            return new THREE.MeshPhongMaterial({ color: 0xffffff, shininess: 5 });
        }
        const matType = matEl.getAttribute('type') || 'lambert';
        const color   = parseInt(matEl.getAttribute('color') || '0xffffff', 16);
        switch (matType) {
            case 'line_basic': return new THREE.LineBasicMaterial({ color, linewidth: parseFloat(matEl.getAttribute('size') || '1') });
            case 'point':      return new THREE.PointsMaterial({ color, size: parseFloat(matEl.getAttribute('size') || '2'), sizeAttenuation: false });
            case 'basic':      return new THREE.MeshBasicMaterial({ color });
            case 'phong':      return new THREE.MeshPhongMaterial({ color, shininess: 5 });
            case 'lambert':
            default:           return new THREE.MeshPhongMaterial({ color, shininess: 5 });
        }
    },

    updateObjectParameters: function(obj, el) {
        const first = (tag) => { const l = el.getElementsByTagName(tag); return l.length > 0 ? l[0] : null; };

        const posEl = first('position');
        if (posEl) {
            const x = parseFloat(posEl.getAttribute('x'));
            const y = parseFloat(posEl.getAttribute('y'));
            const z = parseFloat(posEl.getAttribute('z'));
            if (isFinite(x) && isFinite(y) && isFinite(z)) obj.position.set(x, y, z);
        }

        const rotEl = first('rotate');
        if (rotEl) {
            obj.rotation.set(
                parseFloat(rotEl.getAttribute('x') || '0') * Math.PI / 180,
                parseFloat(rotEl.getAttribute('y') || '0') * Math.PI / 180,
                parseFloat(rotEl.getAttribute('z') || '0') * Math.PI / 180
            );
        }

        const scaEl = first('scale');
        if (scaEl) {
            obj.scale.set(
                parseFloat(scaEl.getAttribute('x') || '1'),
                parseFloat(scaEl.getAttribute('y') || '1'),
                parseFloat(scaEl.getAttribute('z') || '1')
            );
        }

        const latEl = first('lookat');
        if (latEl) {
            const lx = parseFloat(latEl.getAttribute('x')) || 0;
            const ly = parseFloat(latEl.getAttribute('y')) || 0;
            const lz = parseFloat(latEl.getAttribute('z')) || 0;

            // hor_circle: il cerchio è generato su piano XY (normale = Z).
            // Il server manda <lookat> = pdv (il punto di vista sulla superficie).
            // Per allinearlo all'orizzonte reale dobbiamo ruotare l'oggetto
            // in modo che la normale del cerchio (asse Z locale) punti verso pdv
            // (= verso l'esterno della sfera terrestre in quel punto).
            if (obj.name === 'hor_circle') {
                const normal = new THREE.Vector3(lx, ly, lz).normalize();
                const zAxis  = new THREE.Vector3(0, 0, 1);
                const q = new THREE.Quaternion().setFromUnitVectors(zAxis, normal);
                obj.quaternion.copy(q);
            } else {
                obj.lookAt(lx, ly, lz);
            }
        }
    },

    // ── Loop ─────────────────────────────────────────────────────────────────
    onWindowResize: function() {
        this.camera.aspect = window.innerWidth / window.innerHeight;
        this.camera.updateProjectionMatrix();
        this.renderer.setSize(window.innerWidth, window.innerHeight);
    },

    animate: function() {
        requestAnimationFrame(() => this.animate());
        this.renderer.render(this.scene, this.camera);
    }
};
