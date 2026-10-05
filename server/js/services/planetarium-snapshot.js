// Live WebSocket IDs are nonnegative. Temporary calculations use a separate range.
let nextTemporaryId = -1;
export function computeSnapshot(solar, config, query = '{}') {
    const id = nextTemporaryId--;
    if (nextTemporaryId < -2147483648) nextTemporaryId = -1;
    try {
        solar.registerClient(id, query);
        return solar.computeCelestialPositions(id, JSON.stringify({ ...config, save_snapshot: false }));
    } finally {
        solar.unregisterClient(id);
    }
}

export function simulationConfig(message) {
    const lat = Number(message.lat), lon = Number(message.lon);
    if (message.lat == null || message.lon == null || !Number.isFinite(lat) || !Number.isFinite(lon)
        || Math.abs(lat) > 90 || Math.abs(lon) > 180) throw new Error('Coordinate non valide');
    const raw = String(message.data || '');
    // Older clients send an ISO timestamp without a suffix, but the UI is in UTC.
    const date = new Date(/[zZ]|[+-]\d{2}:\d{2}$/.test(raw) ? raw : raw + 'Z');
    if (!Number.isFinite(date.getTime())) throw new Error('Data UTC non valida');
    const [ymd, hms] = date.toISOString().slice(0, 19).split('T');
    return { lookfrom: 'earth', latitude: lat, longitude: lon,
        height: Number.isFinite(Number(message.alt)) ? Number(message.alt) : 0,
        azimut: Number.isFinite(Number(message.az)) ? Number(message.az) : 0,
        date: `${ymd.split('-').reverse().join('-')} ${hms}`, utc: date.toISOString() };
}
