# Changes

## 0.1.0

- Empfang von Wireless-M-Bus-Zählern (Wasser- und Wärmemengenzähler)
  per CC1101-Funkmodul, bis zu 9 Zähler gleichzeitig.
- Verschlüsselte Telegramme werden bei hinterlegtem AES-128-Schlüssel
  automatisch entschlüsselt.
- Pro Kanal einstellbar: Mindestwartezeit zwischen zwei Sendungen,
  Senden nur bei Änderung, Suspendieren, Zählerüberwachung mit
  eigenem Status-Objekt.
- Status-LEDs für Modul und Kanäle nach dem einheitlichen
  OpenKNX-Zustandsmodell.
- MQTT: pro empfangenem Telegramm ein JSON-Objekt mit allen darin
  enthaltenen Messwerten, ohne eigenen ETS-Parameter — die
  Veröffentlichung folgt der globalen MQTT-Einstellung im Netzwerkmodul.
- Konsolenbefehle `wmbus debug`, `wmbus stats` und `wmbus list` für die
  Inbetriebnahme.
