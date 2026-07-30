### Zählerüberwachung

Legt fest, nach welcher Zeit ohne gültiges Telegramm das Status-KO auf **Inaktiv** wechselt.

Der Timer startet beim Gerätestart und wird bei jedem erfolgreich empfangenen und entschlüsselten Telegramm zurückgesetzt. Trifft innerhalb der eingestellten Zeit kein gültiges Telegramm ein – z. B. weil der Zähler außer Reichweite ist oder der Akku leer ist – wechselt der Status auf Inaktiv (0).

Ein AES-Entschlüsselungsfehler setzt den Status ebenfalls sofort auf Inaktiv, unabhängig vom Timer.

Transiente Fehler wie einzelne CRC-Fehler oder unvollständige Telegramme beeinflussen den Status nicht.
