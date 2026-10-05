// Opzioni del racconto e compatibilità con le favole salvate a scelta singola.
// choices:start
export const GENRE_PHRASE = {
    amicizia: 'amicizia',
    avventura: 'avventura',
    coraggio: 'coraggio',
    nanna: 'per addormentarsi',
    magia: 'magia e sogni',
    divertente: 'divertente, con situazioni buffe e tanta allegria',
};

export const SETTING_PHRASE = {
    bosco: 'un bosco magico',
    castello: 'un castello antico',
    acqua: 'mare, fiume e rive',
    montagna: 'un villaggio tra le montagne',
    nuvole: 'una città tra le nuvole',
    cielo: 'il cielo stellato e l\'osservazione delle stelle',
    giardino: 'un giardino segreto o una casa accogliente',
};
// choices:end

function selectedKeys(value, options) {
    const values = Array.isArray(value) ? value : typeof value === 'string' ? value.split(',') : [];
    return [...new Set(values.filter(key => typeof key === 'string')
        .map(key => key.trim()).filter(key => Object.hasOwn(options, key)))];
}

export function normalizeStoryChoices(story) {
    const genres = selectedKeys(story.genres ?? story.genre, GENRE_PHRASE);
    const settingKeys = selectedKeys(story.settingKeys ?? story.settingKey, SETTING_PHRASE);
    return {
        genres, settingKeys,
        // Stringhe compatibili con il formato già usato dall'API e dai report.
        genre: genres.join(', ') || null,
        settingKey: settingKeys.join(', ') || null,
    };
}
