/* The observer and the rendered sky share this state, including after reconnect. */
(function (root) {
    const directions = { N: [0, 0], NE: [45, 0], E: [90, 0], SE: [135, 0],
        S: [180, 0], SW: [225, 0], W: [270, 0], NW: [315, 0], Z: [0, 90] };
    function create(initial = {}) {
        const state = { lat: 45.46, lon: 9.19, az: 0, alt: 0, fov: 70,
            zoom: 1, light: true, horizon: true, running: true,
            data: new Date().toISOString().slice(0, 19) + 'Z', ...initial };
        const numeric = (value) => value !== null && value !== '' && Number.isFinite(Number(value));
        function setLocation(lat, lon) {
            if (!numeric(lat) || !numeric(lon) || Math.abs(Number(lat)) > 90 || Math.abs(Number(lon)) > 180)
                throw new Error('Coordinate non valide');
            state.lat = Number(lat); state.lon = Number(lon);
        }
        function setDateTime(date, time) {
            const value = `${date}T${time.length === 5 ? time + ':00' : time}Z`;
            if (!/^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$/.test(value) || !Number.isFinite(Date.parse(value)))
                throw new Error('Data e ora UTC non valide');
            state.data = value;
        }
        function setView(az, alt) {
            if (numeric(az)) state.az = ((Number(az) % 360) + 360) % 360;
            if (numeric(alt)) state.alt = Math.max(-90, Math.min(90, Number(alt)));
        }
        function trackCommand(command) {
            const [name, ...args] = String(command).trim().split(/\s+/);
            if (name === 'observe' && args.length >= 2) {
                setLocation(args[1], args[0]);
                if (directions[args[2]]) setView(...directions[args[2]]);
            } else if (name === 'date' && /^\d{2}-\d{2}-\d{4}$/.test(args[0]) && args[1]) {
                setDateTime(args[0].split('-').reverse().join('-'), args[1]);
            } else if (name === 'view') {
                if (directions[args[0]]) setView(...directions[args[0]]);
                else if (args.length >= 2) setView(args[0], args[1]);
            } else if (name === 'move' && args.length >= 2) {
                setView(state.az + Number(args[1]), state.alt + Number(args[0]));
            } else if (name === 'azimut') setView(args[0], state.alt);
            else if (name === 'altezza') setView(state.az, args[0]);
            else if (['zoom', 'fov'].includes(name) && numeric(args[0]) && Number(args[0]) > 0)
                state[name] = Number(args[0]);
            else if (name === 'light') state.light = args[0] === 'on';
            else if (name === 'horizont') state.horizon = ['true', 'on', '1'].includes(args[0]);
            else if (name === 'execute_loop') state.running = args[0] === 'on';
        }
        function updateFromXml(doc, lockDate = false) {
            const value = id => doc.querySelector(`[id="${id}"]`)?.getAttribute('value');
            const lat = value('latitudine'), lon = value('longitudine');
            if (numeric(lat) && numeric(lon)) setLocation(lat, lon);
            setView(value('azimut'), value('height'));
            for (const key of ['zoom', 'fov']) {
                const v = doc.querySelector(`camera ${key}`)?.getAttribute('value');
                if (numeric(v) && Number(v) > 0) state[key] = Number(v);
            }
            const date = value('the_date');
            if (date && !lockDate) {
                const match = date.match(/^(\d{1,2})-(\d{1,2})-(\d{4})\s+(\d{1,2}):(\d{1,2}):(\d{1,2})/);
                if (match) {
                    const [, day, month, year, h, m, s] = match;
                    setDateTime(`${year}-${month.padStart(2, '0')}-${day.padStart(2, '0')}`,
                        [h, m, s].map(v => v.padStart(2, '0')).join(':'));
                }
            }
        }
        function restorationCommands() {
            const [date, time] = state.data.replace(/Z$/, '').split('T');
            return ['execute_loop off', `observe ${state.lon} ${state.lat} N`,
                `date ${date.split('-').reverse().join('-')} ${time} UT`,
                `view ${state.az} ${state.alt}`, `fov ${state.fov}`, `zoom ${state.zoom}`,
                `horizont ${state.horizon}`, `light ${state.light ? 'on' : 'off'}`,
                `execute_loop ${state.running ? 'on' : 'off'}`];
        }
        return { setLocation, setDateTime, trackCommand, updateFromXml, restorationCommands,
            snapshot: () => ({ ...state }),
            chatContext: () => ({ lat: state.lat, lon: state.lon, data: state.data,
                az: state.az, alt: state.alt }) };
    }
    root.PlanetariumSimulation = { create };
})(globalThis);
