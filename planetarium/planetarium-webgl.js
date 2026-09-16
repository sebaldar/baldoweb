/**
 * planetarium-webgl.js
 * Gestione scena Three.js: renderer, camera, luci, oggetti, primitives
 */
const webgl = {
    scene:         null,
    camera:        null,
    renderer:      null,
    sunLight:      null,
    click_objects: [],  
    
    _debugVisibility: false,
    _visibilityStates: new Map(),  
    
    // Cache per la texture del mirino così la generiamo una volta sola
    _crosshairTex: null,

    // ── Generatore Texture Mirino (NUOVO) ────────────────────────────────────
    getCrosshairTexture: function() {
        if (this._crosshairTex) return this._crosshairTex;
        
        const canvas = document.createElement('canvas');
        canvas.width = 64;
        canvas.height = 64;
        const ctx = canvas.getContext('2d');
        
        // Lo disegniamo BIANCO puro. Il colore reale (Ciano) glielo darà il Material!
        ctx.strokeStyle = '#ffffff'; 
        ctx.lineWidth = 4;
        
        ctx.beginPath();
        ctx.moveTo(32, 0); ctx.lineTo(32, 20); // Linea Sopra
        ctx.moveTo(32, 44); ctx.lineTo(32, 64); // Linea Sotto
        ctx.moveTo(0, 32); ctx.lineTo(20, 32); // Linea Sinistra
        ctx.moveTo(44, 32); ctx.lineTo(64, 32); // Linea Destra
        ctx.stroke();
        
        this._crosshairTex = new THREE.CanvasTexture(canvas);
        return this._crosshairTex;
    },

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
        
        this.sunLight = new THREE.PointLight(0xffffff, 2.0, 0, 0); 
        this.sunLight.name = 'sunlight';
        this.sunLight.castShadow = false;
        this.sunLight.position.set(0, 0, 0); 
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
    processRenderer: function(rendererEl) {
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
            } catch(e) { }
        };
        
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
                            
                            const timeParts = timePart.split(':');
                            if (timeParts.length >= 2) {
                                const hours = timeParts[0].padStart(2, '0');
                                const minutes = timeParts[1].padStart(2, '0');
                                const seconds = timeParts[2] ? timeParts[2].padStart(2, '0') : '00';
                                timePart = `${hours}:${minutes}:${seconds}`;
                            }
                            
                            const dateParts = datePart.split('-');
                            if (dateParts.length === 3) {
                                const num1 = parseInt(dateParts[0]);
                                const num3 = parseInt(dateParts[2]);
                                
                                if (num1 > 31) {
                                    datePart = `${dateParts[0]}-${dateParts[1].padStart(2,'0')}-${dateParts[2].padStart(2,'0')}`;
                                } else if (num3 > 31) {
                                    datePart = `${dateParts[2]}-${dateParts[1].padStart(2,'0')}-${dateParts[0].padStart(2,'0')}`;
                                } else {
                                    if (num1 <= 12) {
                                        datePart = `${dateParts[2]}-${dateParts[1].padStart(2,'0')}-${dateParts[0].padStart(2,'0')}`;
                                    } else {
                                        datePart = `${dateParts[0]}-${dateParts[1].padStart(2,'0')}-${dateParts[2].padStart(2,'0')}`;
                                    }
                                }
                            }
                            
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
        } catch(e) { }
        
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
        if (name === 'sunlight') {
            this.sunLight.color.setHex(color);
            this.sunLight.intensity = Math.max(intensity * 2.0, 2.0);
            this.sunLight.visible   = !hidden;
            if (hasPosEl && isFinite(px) && isFinite(py) && isFinite(pz)) {
                this.sunLight.position.set(px, py, pz);
            }
            return;
        }
        let light = this.scene.getObjectByName(name, true);
        if (!light) {
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
        }
        light.color.setHex(color);
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
        
        if (obj && primEl.getElementsByTagName('recreate').length > 0) {
            this.scene.remove(obj);
            if (obj.geometry) obj.geometry.dispose();
            const idx = this.click_objects.indexOf(obj);
            if (idx > -1) this.click_objects.splice(idx, 1);
            obj = null;
        }
        
        if (obj && primEl.getElementsByTagName('remove').length > 0) {
            this.scene.remove(obj);
            if (obj.geometry) obj.geometry.dispose();
            const idx = this.click_objects.indexOf(obj);
            if (idx > -1) this.click_objects.splice(idx, 1);
            return;
        }
        
        if (!obj) {
            if (name === 'sky') {
                obj = this.scene.getObjectByName('sky', true);
                if (!obj) return;
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
        
        this.updateObjectParameters(obj, primEl);
        
        const hasHidden = primEl.getElementsByTagName('hidden').length > 0;
        const hasShow = primEl.getElementsByTagName('show').length > 0;
        
        if (hasHidden) {
            if (obj.visible) {
                obj.visible = false;
                this._visibilityStates.set(name, false);
                const idx = this.click_objects.indexOf(obj);
                if (idx > -1) this.click_objects.splice(idx, 1);
            }
        } else if (hasShow) {
            if (!obj.visible) {
                obj.visible = true;
                this._visibilityStates.set(name, true);
                if (primEl.getElementsByTagName('clickable').length > 0) {
                    if (!this.click_objects.includes(obj)) {
                        this.click_objects.push(obj);
                    }
                }
            }
        } else {
            this._visibilityStates.set(name, obj.visible);
        }
    },

    createPrimitiveObject: function(primEl, type, name) {
        let geometry, material;
        const matList = primEl.getElementsByTagName('material');
        const matEl   = matList.length > 0 ? matList[0] : null;

        // ── NUOVA GESTIONE MIRINO (TARGET) ──
        if (type === 'target') {
            material = this.createMaterial(matEl, type);
            // I target (mirini) diventano Sprite (sempre rivolti alla camera)
            const sprite = new THREE.Sprite(material);
            
            // Il C++ manda la posizione nel tag <vertice>
            const vertList = primEl.getElementsByTagName('vertice');
            if (vertList.length > 0) {
                const parts = vertList[0].textContent.trim().split(/[\s,]+/);
                if (parts.length >= 3) {
                    sprite.position.set(parseFloat(parts[0]), parseFloat(parts[1]), parseFloat(parts[2]));
                }
            }
            
            // Scala dello Sprite in base a 'size' (moltiplicatore ridotto perché in world units)
            const size = matEl ? parseFloat(matEl.getAttribute('size') || '20') : 20;
            const scaleMult = 0.05; // Modifica questo valore se risulta troppo grande/piccolo
            sprite.scale.set(size * scaleMult, size * scaleMult, 1);
            
            return sprite;
        }

        // ── RESTO DEGLI OGGETTI (Sphere, Point, Line) ──
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
        
        if (type === 'sphere' && matEl) {
            const mapPath = matEl.getAttribute('map');
            if (mapPath) {
                const url = mapPath.replace('/my_image/', '/public/');
                const tex = new THREE.TextureLoader().load(url, () => {}, undefined, () => {});
                if (name === 'sun') {
                    material = new THREE.MeshBasicMaterial({ map: tex });
                } else {
                    material = new THREE.MeshPhongMaterial({ map: tex, shininess: 8 });
                }
            } else if (name !== 'sun') {
                const color = matEl ? parseInt(matEl.getAttribute('color') || '0xffffff', 16) : 0xffffff;
                material = new THREE.MeshPhongMaterial({ color, shininess: 8 });
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
            // ── NUOVO MATERIALE MIRINO ──
            case 'crosshair':  
                return new THREE.SpriteMaterial({ 
                    map: this.getCrosshairTexture(), 
                    color: color, 
                    transparent: true, 
                    depthTest: false, // Per renderlo sempre visibile in primo piano
                    depthWrite: false 
                });
                
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

        // Se è un target/sprite non leggiamo <position> perché la prendiamo da <vertice> 
        // Tuttavia, le primitive normali usano <position>.
        const posEl = first('position');
        if (posEl && obj.type !== 'Sprite') {
            const x = parseFloat(posEl.getAttribute('x'));
            const y = parseFloat(posEl.getAttribute('y'));
            const z = parseFloat(posEl.getAttribute('z'));
            if (isFinite(x) && isFinite(y) && isFinite(z)) obj.position.set(x, y, z);
        }

        const rotEl = first('rotate');
        if (rotEl && obj.type !== 'Sprite') { // Gli sprite ruotano da soli
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
        if (latEl && obj.type !== 'Sprite') {
            const lx = parseFloat(latEl.getAttribute('x')) || 0;
            const ly = parseFloat(latEl.getAttribute('y')) || 0;
            const lz = parseFloat(latEl.getAttribute('z')) || 0;

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
