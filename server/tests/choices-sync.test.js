import test from 'node:test';
import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import { readFileSync } from 'node:fs';

const tool = new URL('../../ilpiccolobardo/tools/sync-choices.mjs', import.meta.url);
const choices = JSON.parse(readFileSync(new URL('../../ilpiccolobardo/choices.json', import.meta.url), 'utf8'));

test('HTML, JS and FastAPI choices are generated from choices.json', () => {
    const run = spawnSync(process.execPath, [tool.pathname, '--check'], { encoding: 'utf8' });
    assert.equal(run.status, 0, run.stderr);
});

test('every choice has a unique key, a label and a prompt phrase', () => {
    for (const [group, options] of Object.entries(choices)) {
        assert.equal(new Set(options.map(o => o.key)).size, options.length, group);
        for (const option of options) assert.ok(option.key && option.label && option.phrase, `${group}:${option.key}`);
    }
});
