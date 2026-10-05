import test from 'node:test';
import assert from 'node:assert/strict';
import { skyDirection } from '../js/services/sky-directions.js';

test('native azimuth maps to compass sectors without inventing missing data', () => {
    for (const [angle, direction] of [[0, 'nord'], [45, 'nord-est'], [90, 'est'], [180, 'sud'], [225, 'sud-ovest'], [270, 'ovest'], [360, 'nord'], [-90, 'ovest'], [22.49, 'nord'], [22.5, 'nord-est']]) {
        assert.equal(skyDirection(angle).direzione_cardinale, direction);
    }
    for (const value of [null, undefined, NaN, Infinity, '90']) assert.deepEqual(skyDirection(value), {});
    assert.equal(skyDirection(359.999).azimut_deg, 0);
});
