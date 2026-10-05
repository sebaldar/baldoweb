// Titoli e creazione del PDF nel browser.
import { appState } from './state.js';
import { sendMessage } from './connection.js';

export const TITOLO_PREDEFINITO = 'La mia favola';

// ------------------------------------------------------------
// Stampa PDF
// ------------------------------------------------------------

// Il titolo lo chiede al server (una favola non ne ha uno proprio)
// e lo tiene in cache su currentStory, così non lo richiede due
// volte per la stessa storia. Se il server non risponde in tempo,
// dopo 12s usa un titolo di riserva: meglio un titolo generico che
// un pulsante bloccato per sempre.
export function richiediTitolo(testo) {
    return new Promise((resolve) => {
        let risolto = false;
        const finisci = (titolo) => {
            if (risolto) return;
            risolto = true;
            appState.titoloResolver = null;
            resolve(titolo);
        };
        appState.titoloResolver = finisci;
        sendMessage('generate_title', { text: testo });
        setTimeout(() => finisci('La mia favola'), 12000);
    });
}

export function arrayBufferABase64(buffer) {
    let binario = '';
    const byte = new Uint8Array(buffer);
    const dimensioneBlocco = 0x8000;
    for (let i = 0; i < byte.length; i += dimensioneBlocco) {
        binario += String.fromCharCode.apply(null, byte.subarray(i, i + dimensioneBlocco));
    }
    return btoa(binario);
}

// Font "Comic Neue" (alternativa libera e più curata di Comic Sans,
// coerente con lo stile del sito) — jsPDF include solo Helvetica/
// Times/Courier di base, serve incorporarne uno adatto a una storia
// per bambini. Scaricato una sola volta, poi tenuto in cache.
export async function caricaFontComicNeue() {
    if (appState.comicNeueRegularB64 && appState.comicNeueBoldB64) return;
    const [bufRegular, bufBold] = await Promise.all([
        fetch('/public/fonts/ComicNeue-Regular.ttf').then(r => r.arrayBuffer()),
        fetch('/public/fonts/ComicNeue-Bold.ttf').then(r => r.arrayBuffer()),
    ]);
    appState.comicNeueRegularB64 = arrayBufferABase64(bufRegular);
    appState.comicNeueBoldB64 = arrayBufferABase64(bufBold);
}

export function immagineComeDataURL(url) {
    return fetch(url)
        .then(r => r.blob())
        .then(blob => new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.onloadend = () => resolve(reader.result);
            reader.onerror = reject;
            reader.readAsDataURL(blob);
        }));
}

export function dimensioniImmagine(dataUrl) {
    return new Promise((resolve, reject) => {
        const img = new Image();
        img.onload = () => resolve({ larghezza: img.naturalWidth, altezza: img.naturalHeight });
        img.onerror = reject;
        img.src = dataUrl;
    });
}

export async function printStory() {
    if (!appState.currentStory || !appState.currentStory.text) {
        showToast('Non c\'è ancora una storia da stampare!', 'warning');
        return;
    }

    const printBtn = document.getElementById('printBtn');
    const testoOriginaleBtn = printBtn.textContent;
    printBtn.disabled = true;
    printBtn.textContent = '⏳ Preparo il PDF...';

    try {
        if (!appState.currentStory.title) {
            appState.currentStory.title = await richiediTitolo(appState.currentStory.text);
        }
        await caricaFontComicNeue();

        const doc = new jspdf.jsPDF({ unit: 'mm', format: 'a4' });
        doc.addFileToVFS('ComicNeue-Regular.ttf', appState.comicNeueRegularB64);
        doc.addFont('ComicNeue-Regular.ttf', 'ComicNeue', 'normal');
        doc.addFileToVFS('ComicNeue-Bold.ttf', appState.comicNeueBoldB64);
        doc.addFont('ComicNeue-Bold.ttf', 'ComicNeue', 'bold');

        const larghezzaPagina = doc.internal.pageSize.getWidth();
        const altezzaPagina = doc.internal.pageSize.getHeight();
        const margine = 22;
        const larghezzaUtile = larghezzaPagina - margine * 2;
        let y = margine;

        // Titolo
        doc.setFont('ComicNeue', 'bold');
        doc.setFontSize(24);
        doc.setTextColor(102, 126, 234); // #667eea, lo stesso viola del sito
        doc.splitTextToSize(appState.currentStory.title, larghezzaUtile).forEach(riga => {
            doc.text(riga, larghezzaPagina / 2, y, { align: 'center' });
            y += 10;
        });
        y += 3;

        // Filo decorativo sotto il titolo
        doc.setDrawColor(102, 126, 234);
        doc.setLineWidth(0.8);
        doc.line(margine, y, larghezzaPagina - margine, y);
        y += 10;

        // Illustrazione, se la storia ne ha una
        if (appState.currentStory.illustration) {
            try {
                const dataUrl = await immagineComeDataURL(appState.currentStory.illustration);
                const dimensioni = await dimensioniImmagine(dataUrl);
                const larghezzaImg = Math.min(110, larghezzaUtile);
                const altezzaImg = larghezzaImg * (dimensioni.altezza / dimensioni.larghezza);
                if (y + altezzaImg > altezzaPagina - margine) {
                    doc.addPage();
                    y = margine;
                }
                doc.addImage(dataUrl, 'PNG', (larghezzaPagina - larghezzaImg) / 2, y, larghezzaImg, altezzaImg);
                y += altezzaImg + 10;
            } catch (e) {
                console.warn('Illustrazione non inclusa nel PDF:', e);
            }
        }

        // Corpo della storia, paragrafo per paragrafo
        doc.setFont('ComicNeue', 'normal');
        doc.setFontSize(13);
        doc.setTextColor(40, 40, 40);
        const interlinea = 7;
        const testoPulito = appState.currentStory.text.replace(/<[^>]+>/g, '');

        testoPulito.split(/\n\s*\n/).forEach(paragrafo => {
            const pulito = paragrafo.trim();
            if (!pulito) return;
            doc.splitTextToSize(pulito, larghezzaUtile).forEach(riga => {
                if (y > altezzaPagina - margine) {
                    doc.addPage();
                    y = margine;
                }
                doc.text(riga, margine, y);
                y += interlinea;
            });
            y += interlinea * 0.6;
        });

        // Piè di pagina su ogni pagina
        const numPagine = doc.internal.getNumberOfPages();
        for (let i = 1; i <= numPagine; i++) {
            doc.setPage(i);
            doc.setFont('ComicNeue', 'normal');
            doc.setFontSize(9);
            doc.setTextColor(160, 160, 160);
            doc.text('Il Piccolo Bardo', larghezzaPagina / 2, altezzaPagina - 10, { align: 'center' });
        }

        const nomeFile = (appState.currentStory.title || 'storia')
            .normalize('NFD').replace(/[̀-ͯ]/g, '')
            .replace(/[^\w\s-]/g, '').trim().replace(/\s+/g, '_').slice(0, 60) || 'storia';
        doc.save(`${nomeFile}.pdf`);

    } catch (e) {
        console.error('Errore nella creazione del PDF:', e);
        showToast('Non sono riuscito a creare il PDF. Riprova tra poco.', 'error');
    } finally {
        printBtn.disabled = false;
        printBtn.textContent = testoOriginaleBtn;
    }
}
