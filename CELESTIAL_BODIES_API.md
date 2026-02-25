# API Corpi Celesti - Client/Server

## Comandi Client → Server

### Nascondere un corpo celeste
```javascript
sendCommand('sun hide');    // o 'sun hidden'
sendCommand('moon hide');
sendCommand('mars hide');
```

Il server risponde con:
```xml
<?xml version="1.0" encoding="UTF-8" ?>
<results>
    <renderer>
        <primitive name="sun">
            <hidden />
        </primitive>
    </renderer>
</results>
```

### Mostrare un corpo celeste
```javascript
sendCommand('sun show');
sendCommand('moon show');
```

Il server risponde con:
```xml
<?xml version="1.0" encoding="UTF-8" ?>
<results>
    <renderer>
        <primitive name="sun">
            <show />
        </primitive>
    </renderer>
</results>
```

### Ottenere informazioni su un corpo celeste
```javascript
sendCommand('sun');      // Senza parametri
sendCommand('mars');
sendCommand('jupiter');
```

Il server risponde con informazioni dettagliate:
```xml
<?xml version="1.0" encoding="UTF-8" ?>
<results>
    <terminal>
sun in Aquarius [Pisces]
    Dec=-8.234° AR=22h 45m 12s R=1.0
    Dec=-8.234 AR=341.3 R=1.0
    </terminal>
    <dialog>
        <h3>☀️ Sun</h3>
        <p><strong>Costellazione:</strong> Aquarius</p>
        <p><strong>Zodiaco:</strong> Pisces</p>
        <p><strong>Declinazione:</strong> -8.234°</p>
        <p><strong>Ascensione Retta:</strong> 22h 45m 12s (341.3°)</p>
        <p><strong>Distanza:</strong> 1.0 AU</p>
    </dialog>
</results>
```

## Gestione Client

Il client `planetarium-webgl.js` processa automaticamente:

1. **Tag `<hidden />`**: Imposta `obj.visible = false`
2. **Tag `<show />`**: Imposta `obj.visible = true`
3. **Tag `<dialog>`**: Mostra popup con informazioni (già implementato in planetarium.html)

## Esempio Click su Corpo Celeste

Quando l'utente clicca su un corpo celeste (es. sole):

1. **Client invia:** `mouse 0 sun x y z distance down`
2. **Server risponde:** XML con `<terminal>` e `<dialog>`
3. **Client mostra:** Dialog popup con arrow che punta al sole

## Comandi Disponibili per Corpi Celesti

Tutti i corpi celesti nel sistema supportano:
- `<nome> hide` - Nasconde il corpo
- `<nome> show` - Mostra il corpo
- `<nome>` - Mostra informazioni in dialog

Corpi tipici:
- sun, moon, mercury, venus, mars, jupiter, saturn, uranus, neptune, pluto
- earth (per riferimento)
- stelle e altri oggetti definiti nel sistema
