import { defineConfig, devices } from '@playwright/test';

export default defineConfig({
    testDir: './e2e',
    testMatch: '*.spec.js',
    reporter: 'list',
    use: { baseURL: 'http://127.0.0.1:4173', serviceWorkers: 'block', locale: 'it-IT' },
    projects: [
        { name: 'desktop', use: { ...devices['Desktop Chrome'] } },
        { name: 'mobile', use: { ...devices['Pixel 7'] } },
    ],
    webServer: { command: 'node e2e/static-server.mjs', url: 'http://127.0.0.1:4173/', reuseExistingServer: !process.env.CI },
});
