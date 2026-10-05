import { test, expect } from '@playwright/test';

const WS_URL = 'wss://www.ilpiccolobardo.it/ws';

// Sostituisce server Node e FastAPI: risponde a generate_story come farebbe
// il backend, con gli stessi eventi (nodo_start, token, fine).
async function mockBackend(page, { reply } = {}) {
    const sent = [];
    await page.route('**/api/session', route => route.fulfill({ json: { session: 'test-session' } }));
    await page.routeWebSocket(WS_URL, ws => {
        ws.onMessage(raw => {
            const message = JSON.parse(raw);
            sent.push(message);
            if (message.action !== 'generate_story') return;
            for (const event of reply?.(message) ?? [
                { tipo: 'nodo_start', nodo: 'analizza_prompt' },
                { tipo: 'token', testo: 'C\'era una volta ' },
                { tipo: 'token', testo: 'un orsetto coraggioso.' },
                { tipo: 'fine', racconto: 'C\'era una volta un orsetto coraggioso.', storia_id: 'abc', iterazioni: 1 },
            ]) ws.send(JSON.stringify(event));
        });
    });
    return sent;
}

async function openApp(page, options) {
    const sent = await mockBackend(page, options);
    await page.goto('/');
    await expect(page.locator('#connectionStatus')).toContainText('Connesso');
    return sent;
}

const chip = (page, group, key) => page.locator(`#${group}Chips [data-key="${key}"]`);

test('renders every choice chip defined in choices.json', async ({ page }) => {
    const { genre, setting } = await (await page.request.get('/choices.json')).json();
    await openApp(page);
    for (const [group, options] of Object.entries({ genre, setting })) {
        await expect(page.locator(`#${group}Chips .chip`)).toHaveCount(options.length);
        for (const { key, label } of options) await expect(chip(page, group, key)).toHaveText(label);
    }
});

test('a child story is generated from the form choices', async ({ page }) => {
    const sent = await openApp(page);
    await page.locator('.age-btn', { hasText: '5 anni' }).click();
    await page.fill('#childName', 'Ada');
    for (const key of ['amicizia', 'divertente']) await chip(page, 'genre', key).click();
    await chip(page, 'setting', 'bosco').click();
    await chip(page, 'setting', 'cielo').click();
    await expect(chip(page, 'genre', 'divertente')).toHaveAttribute('aria-pressed', 'true');
    await page.click('#generateBtn');

    await expect(page.locator('#storyDisplay')).toContainText('un orsetto coraggioso');
    await expect(page.locator('#readBtn')).toBeEnabled();

    const request = sent.find(message => message.action === 'generate_story').data;
    expect(request.age).toBe(5);
    expect(request.nome).toBe('Ada');
    expect(request.genere).toBe('amicizia, divertente');
    expect(request.ambientazione).toBe('bosco, cielo');
    expect(request.userPrompt).toContain('divertente, con situazioni buffe e tanta allegria');
});

test('generation requires the age and does not contact the backend without it', async ({ page }) => {
    const sent = await openApp(page);
    await page.click('#generateBtn');
    await expect(page.locator('.toast, [role="status"]').filter({ hasText: 'età' }).first()).toBeVisible();
    expect(sent.filter(message => message.action === 'generate_story')).toHaveLength(0);
});

test('a refused prompt shows the reason instead of an empty story', async ({ page }) => {
    await openApp(page, { reply: () => [
        { tipo: 'nodo_start', nodo: 'valuta_prompt' },
        { tipo: 'fine', racconto: '', errore: 'prompt non adatto', motivo_rifiuto: 'la richiesta contiene contenuti non adatti ai bambini' },
    ] });
    await page.locator('.age-btn', { hasText: '4 anni' }).click();
    await page.fill('#userPrompt', 'una storia')
    await page.click('#generateBtn');
    await expect(page.locator('#storyDisplay [role="alert"]')).toContainText('non adatti ai bambini');
    await expect(page.locator('#storyDisplay').getByRole('button', { name: /Riprova/ })).toHaveCount(0);
});

test('the story can be saved among the favorites and survives a reload', async ({ page }) => {
    await openApp(page);
    await page.locator('.age-btn', { hasText: '6 anni' }).click();
    await chip(page, 'genre', 'magia').click();
    await page.click('#generateBtn');
    await expect(page.locator('#favoriteBtn')).toBeVisible();
    await expect(page.locator('#favoriteBtn')).toBeEnabled();
    await page.click('#favoriteBtn');
    await expect(page.locator('#favoriteBtn')).toHaveAttribute('aria-pressed', 'true');

    const favorites = () => page.evaluate(() => JSON.parse(localStorage.getItem('bardo_favorites') ?? '[]'));
    expect(await favorites()).toHaveLength(1);
    await page.reload();
    await expect(page.locator('#connectionStatus')).toContainText('Connesso');
    expect(await favorites()).toHaveLength(1);
});
