// Applica il tema prima che venga mostrato il contenuto della pagina.
// Tema scuro: scelta salvata, altrimenti quella del dispositivo. Applicato prima del rendering per evitare lampi.
(function () {
    let saved = null;
    try { saved = localStorage.getItem('bardo_theme'); } catch (e) {}
    const dark = saved ? saved === 'dark' : window.matchMedia('(prefers-color-scheme: dark)').matches;
    if (dark) document.body.classList.add('nightmode');
})();
