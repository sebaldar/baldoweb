import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import { contextFor } from './helpers/browser-context.js';

const payload = '<img src=x onerror="alert(1)">';
test('fragment editor renders stored markup as text', async () => {
    const { context, elements } = await contextFor('admin/manager.html');
    context.displayFragments([{ id: payload, text: payload, setting: payload, tema: payload }]);
    const html = elements.get('fragmentsList').children[0].innerHTML;
    assert.ok(!html.includes('<img'));
    assert.ok(html.includes('&lt;img'));
});

test('archive loader escapes fragment text and related entities', async () => {
    const { context, elements } = await contextFor('loader/frammenti.js');
    context.renderGrid([{ id: payload, text: payload, characters: [payload], emotions: [payload] }]);
    assert.ok(!elements.get('fragment-list').innerHTML.includes('<img'));
    assert.ok(elements.get('fragment-list').innerHTML.includes('&lt;img'));
});

test('legacy story list keeps arbitrary IDs out of inline JavaScript', async () => {
    const { context, elements } = await contextFor('admin/index.html');
    context.displayStoriesList([{ id: "');alert(1);//", title: payload, description: payload }]);
    const html = elements.get('storiesList').children[0].innerHTML;
    assert.ok(!html.includes('<img'));
    assert.ok(!html.includes('onclick='));
    assert.ok(html.includes('data-id="&#39;);alert(1);//"'));
});

test('public stories escape model output while preserving line breaks', async () => {
    const { context, elements } = await contextFor('index.html');
    vm.runInContext('appState.currentStory = {};', context);
    context.displayStory(`${payload}\nSeconda riga`);
    const html = elements.get('storyDisplay').innerHTML;
    assert.ok(!html.includes('<img'));
    assert.ok(html.includes('&lt;img'));
    assert.ok(html.includes('<br>Seconda riga'));
});
