// Native Horizont azimuth: north=0, east=90, south=180, west=270.
export function skyDirection(azimuth) {
    if (typeof azimuth !== 'number' || !Number.isFinite(azimuth)) return {};
    const normalized = ((azimuth % 360) + 360) % 360;
    const directions = ['nord', 'nord-est', 'est', 'sud-est', 'sud', 'sud-ovest', 'ovest', 'nord-ovest'];
    return {
        azimut_deg: Math.round(normalized * 100) / 100 % 360,
        direzione_cardinale: directions[Math.floor((normalized + 22.5) / 45) % 8],
    };
}
