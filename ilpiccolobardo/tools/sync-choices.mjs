// Fonte unica delle scelte del form: choices.json.
// Rigenera i blocchi marcati in index.html, js/choices.js e
// fastapi/services/story_choices.py. Con --check non scrive: esce con
// errore se qualcuno dei tre è fuori sincronia.
import { readFileSync, writeFileSync } from 'node:fs';

const root = new URL('../', import.meta.url);
const choices = JSON.parse(readFileSync(new URL('choices.json', root), 'utf8'));
const checkOnly = process.argv.includes('--check');

const json = value => JSON.stringify(value);
const htmlEscape = text => text.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/"/g, '&quot;');
const jsKey = key => (/^[a-z_$][\w$]*$/i.test(key) ? key : json(key));
const jsString = text => `'${text.replace(/\\/g, '\\\\').replace(/'/g, "\\'")}'`;

const chips = group => choices[group].map(({ key, label }) =>
    `<button class="chip" aria-pressed="false" data-key="${htmlEscape(key)}" data-on-click="pickChip" data-arg="${group}">${htmlEscape(label)}</button>`);

const phrases = (name, group) => [
    `export const ${name} = {`,
    ...choices[group].map(({ key, phrase }) => `    ${jsKey(key)}: ${jsString(phrase)},`),
    '};',
];

const targets = [
    ['index.html', 'genre', /^( *)<!-- choices:genre:start -->\n(?:[\s\S]*?\n)? *<!-- choices:genre:end -->/m,
        indent => [`<!-- choices:genre:start -->`, ...chips('genre'), `<!-- choices:genre:end -->`], true],
    ['index.html', 'setting', /^( *)<!-- choices:setting:start -->\n(?:[\s\S]*?\n)? *<!-- choices:setting:end -->/m,
        indent => [`<!-- choices:setting:start -->`, ...chips('setting'), `<!-- choices:setting:end -->`], true],
    ['js/choices.js', 'phrases', /^()\/\/ choices:start\n(?:[\s\S]*?\n)?\/\/ choices:end/m,
        () => ['// choices:start', ...phrases('GENRE_PHRASE', 'genre'), '', ...phrases('SETTING_PHRASE', 'setting'), '// choices:end'], false],
    ['../fastapi/services/story_choices.py', 'ids', /^()# choices:start\n(?:[\s\S]*?\n)?# choices:end/m,
        () => [
            '# choices:start',
            `GENERI_AMMESSI = {${choices.genre.map(c => json(c.key)).join(', ')}}`,
            `AMBIENTAZIONI_AMMESSE = {${choices.setting.map(c => json(c.key)).join(', ')}}`,
            '# choices:end',
        ], false],
];

const stale = [];
const files = new Map();
for (const [file, name, pattern, build, indented] of targets) {
    const url = new URL(file, root);
    const text = files.get(file) ?? readFileSync(url, 'utf8');
    const match = pattern.exec(text);
    if (!match) throw new Error(`${file}: marcatori "${name}" non trovati`);
    const indent = indented ? match[1] : '';
    const block = build(indent).map(line => (line ? indent + line : line)).join('\n');
    files.set(file, text.replace(pattern, () => block));
}
for (const [file, next] of files) {
    const url = new URL(file, root);
    if (readFileSync(url, 'utf8') === next) continue;
    if (checkOnly) stale.push(file);
    else writeFileSync(url, next);
}
if (stale.length) {
    console.error(`Fuori sincronia con choices.json: ${stale.join(', ')}. Esegui: node ilpiccolobardo/tools/sync-choices.mjs`);
    process.exit(1);
}
