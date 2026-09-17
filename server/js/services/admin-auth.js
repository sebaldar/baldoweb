import { createHash, timingSafeEqual } from 'node:crypto';

// A session identifies a visitor; it does not grant administrative privileges.
const PUBLIC_ACTIONS = new Set([
    'generate_story', 'synthesize_speech', 'generate_title', 'generate_illustration',
]);

export function isAdminToken(token, expected = process.env.ADMIN_TOKEN) {
    if (typeof expected !== 'string' || expected.length < 32 || typeof token !== 'string') return false;
    const digest = value => createHash('sha256').update(value).digest();
    return timingSafeEqual(digest(token), digest(expected));
}

export function canRunAction(action, token) {
    return PUBLIC_ACTIONS.has(action) || isAdminToken(token);
}

export function bearerToken(request) {
    const value = request.headers.authorization;
    return typeof value === 'string' && value.startsWith('Bearer ') ? value.slice(7) : undefined;
}
