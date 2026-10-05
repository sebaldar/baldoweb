/* Service worker di Il Piccolo Bardo.
 * Mette in cache solo la "shell" statica (pagina, script, font, icone).
 * Non tocca mai /api, /ws, /admin, /loader, né immagini e audio generati. */
const CACHE = 'bardo-shell-v13';
const SHELL = [
    '/',
    '/styles/app.css',
    '/js/theme-init.js',
    '/js/app.js',
    '/js/state.js',
    '/js/choices.js',
    '/js/connection.js',
    '/js/generation.js',
    '/js/favorites.js',
    '/js/reading.js',
    '/js/pdf.js',
    '/js/illustrations.js',
    '/js/account.js',
    '/js/ui.js',
    '/js/pwa.js',
    '/safe-html.js',
    '/toast.js',
    '/confirm-dialog.js',
    '/manifest.webmanifest',
    '/public/vendor/jspdf.umd.min.js',
    '/public/fonts/ComicNeue-Regular.ttf',
    '/public/fonts/ComicNeue-Bold.ttf',
    '/public/icons/icon-192.png',
    '/public/icons/icon-512.png',
    '/public/icons/apple-touch-icon.png',
];
const NEVER_CACHE = /^\/(api|ws|admin|loader|\.well-known)(\/|$)|^\/public\/(images|audio)\//;

self.addEventListener('install', (event) => {
    event.waitUntil(caches.open(CACHE).then((cache) => cache.addAll(SHELL)).then(() => self.skipWaiting()));
});

self.addEventListener('activate', (event) => {
    event.waitUntil(
        caches.keys()
            .then((keys) => Promise.all(keys.filter((k) => k !== CACHE).map((k) => caches.delete(k))))
            .then(() => self.clients.claim())
    );
});

self.addEventListener('fetch', (event) => {
    const req = event.request;
    if (req.method !== 'GET') return;
    const url = new URL(req.url);
    if (url.origin !== self.location.origin || NEVER_CACHE.test(url.pathname)) return;

    // Pagina: sempre la versione più recente se c'è rete, altrimenti quella in cache
    if (req.mode === 'navigate') {
        event.respondWith(
            fetch(req)
                .then((res) => {
                    if (res.ok) { const copy = res.clone(); caches.open(CACHE).then((c) => c.put('/', copy)); }
                    return res;
                })
                .catch(() => caches.match('/'))
        );
        return;
    }

    // Risorse statiche: dalla cache, aggiornate in background
    event.respondWith(
        caches.match(req).then((cached) => {
            const network = fetch(req)
                .then((res) => {
                    if (res.ok) { const copy = res.clone(); caches.open(CACHE).then((c) => c.put(req, copy)); }
                    return res;
                })
                .catch(() => cached);
            return cached || network;
        })
    );
});
