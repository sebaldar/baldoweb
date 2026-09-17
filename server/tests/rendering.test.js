import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import vm from 'node:vm';

const site = new URL('../../ilpiccolobardo/', import.meta.url);
function contextFor(file) {
    const elements = new Map();
    const element = () => ({
        innerHTML: '', textContent: '', children: [], style: {},
        appendChild(child) { this.children.push(child); },
        querySelectorAll() { return []; },
        classList: { add() {}, remove() {}, toggle() {} },
    });
    const context = vm.createContext({
        console, window: { addEventListener() {} }, navigator: {},
        document: {
            addEventListener() {}, querySelectorAll() { return []; },
            createElement: element,
            getElementById(id) { if (!elements.has(id)) elements.set(id, element()); return elements.get(id); },
        },
    });
    vm.runInContext(readFileSync(new URL('safe-html.js', site), 'utf8'), context);
    const source = readFileSync(new URL(file, site), 'utf8');
    const scripts = file.endsWith('.html') ? [...source.matchAll(/<script\b[^>]*>([\s\S]*?)<\/script>/g)].map(m => m[1]).join('\n') : source;
    vm.runInContext(scripts, context);
    return { context, elements };
}

const payload = '<img src=x onerror="alert(1)">';
test('fragment editor renders stored markup as text', () => {
    const { context, elements } = contextFor('admin/manager.html');
    context.displayFragments([{ id: payload, text: payload, setting: payload, tema: payload }]);
    const html = elements.get('fragmentsList').children[0].innerHTML;
    assert.ok(!html.includes('<img'));
    assert.ok(html.includes('&lt;img'));
});

test('archive loader escapes fragment text and related entities', () => {
    const { context, elements } = contextFor('loader/frammenti.js');
    context.renderGrid([{ id: payload, text: payload, characters: [payload], emotions: [payload] }]);
    assert.ok(!elements.get('fragment-list').innerHTML.includes('<img'));
    assert.ok(elements.get('fragment-list').innerHTML.includes('&lt;img'));
});

test('legacy story list keeps arbitrary IDs out of inline JavaScript', () => {
    const { context, elements } = contextFor('admin/index.html');
    context.displayStoriesList([{ id: "');alert(1);//", title: payload, description: payload }]);
    const html = elements.get('storiesList').children[0].innerHTML;
    assert.ok(!html.includes('<img'));
    assert.ok(!html.includes('onclick='));
    assert.ok(html.includes('data-id="&#39;);alert(1);//"'));
});

test('public stories escape model output while preserving line breaks', () => {
    const { context, elements } = contextFor('index.html');
    vm.runInContext('currentStory = {}; updateFavoriteButton = () => {};', context);
    context.displayStory(`${payload}\nSeconda riga`);
    const html = elements.get('storyDisplay').innerHTML;
    assert.ok(!html.includes('<img'));
    assert.ok(html.includes('&lt;img'));
    assert.ok(html.includes('<br>Seconda riga'));
});
