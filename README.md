# OFM-WirelessMBus

Empfängt Wireless M-Bus Telegramme (wMBus) von Wasser- und Wärmemengenzählern über ein CC1101-Funkmodul und sendet die Messwerte auf den KNX-Bus.

## Funktionsumfang

Jeder Kanal ist über die eindeutige Meter-ID fest einem Zähler zugeordnet; eingehende Telegramme werden anhand dieser ID dem passenden Kanal zugewiesen. Verschlüsselte Telegramme werden bei hinterlegtem AES-128-Schlüssel automatisch entschlüsselt.

Pro Kanal werden je nach Zählertyp unterschiedliche Werte bereitgestellt:

- **Wasserzähler** – Volumen (m³), Durchfluss (m³/h), Vorlauftemperatur
- **Wärmemengenzähler** – zusätzlich Energie (kWh), Leistung (W) sowie wahlweise Vor-/Rücklauftemperatur oder die Temperaturdifferenz zwischen beiden

Jeder Kanal hat außerdem ein eigenes Status-KO, das anzeigt, ob innerhalb der eingestellten Überwachungszeit (Zählerüberwachung) ein gültiges Telegramm empfangen wurde. Zusätzlich lassen sich pro Kanal eine Mindestwartezeit zwischen zwei Sendungen und "Nur senden bei Änderung" konfigurieren.

Die Funkparameter (Empfangsmodus T1/C1A/C1B/S1 bzw. Kombimodus T1+C1B, Verstärkung, Bandbreite, Präambel-Qualitätsschwelle) gelten global für alle Kanäle. Details zu den einzelnen Parametern stehen in [doc/Applikationsbeschreibung-WirelessMBus.md](doc/Applikationsbeschreibung-WirelessMBus.md).

## Hardware

Das Modul erwartet ein CC1101-Funkmodul an SPI. Die Pins werden nicht im OFM selbst festgelegt, sondern vom einbindenden Projekt über Makros vorgegeben (`WMBUS_CC1101_CS`, `WMBUS_CC1101_GDO0`, `WMBUS_SPI`, optional `WMBUS_SPI_SCK`/`WMBUS_SPI_MOSI`/`WMBUS_SPI_MISO`).

## MQTT

Ist im Netzwerkmodul MQTT aktiviert, veröffentlicht jeder Kanal zusätzlich pro empfangenem Telegramm ein JSON-Objekt mit allen darin enthaltenen Messwerten (je nach Zählertyp `volume`, `volume_flow`, `energy`, `power`, `flow_temp`, `return_temp`, `temp_diff`) unter dem Topic `openknx/<geräte-prefix>/wmbus/<meter-id-hex>`. Einen eigenen ETS-Parameter gibt es dafür nicht — die Veröffentlichung folgt automatisch der globalen MQTT-Einstellung im Netzwerkmodul. Details in [doc/Applikationsbeschreibung-WirelessMBus.md](doc/Applikationsbeschreibung-WirelessMBus.md).

## Status-LEDs

Modul und jeder Kanal können eine Status-LED anzeigen, nach dem Zustandsmodell der OpenKNX-Wiki-Seite "Status-LED":

| Zustand | Farb-LED | Einfarbige LED |
|---|---|---|
| Gerät unkonfiguriert | Aus | Aus |
| Funkmodul nicht erkannt | Rot | Aus |
| Noch keine Zählerdaten erhalten | Gelb | Aus |
| Zählerüberwachung schlägt Alarm | Rot blinkend | Blinkend |
| Zähler in Ordnung | Grün | An |

Beim Gesamtstatus des Moduls gilt: Grün nur, wenn *alle* konfigurierten Zähler in Ordnung sind; ein Alarm bei *einem* Zähler reicht, um das Modul auf Alarm zu setzen.

## Konsolenbefehle

Über die Gerätekonsole lässt sich der Empfang live beobachten:

- **wmbus list** – zeigt alle bisher empfangenen Zähler mit ID, Hersteller, Typ, Verschlüsselungsstatus und Empfangsstärke (RSSI). Praktisch, um die Meter-ID eines Zählers ohne externen Scanner zu ermitteln.
- **wmbus debug** – schaltet eine fortlaufende Anzeige aller empfangenen Rohtelegramme ein bzw. aus.
- **wmbus stats** – zeigt Kennzahlen zur Funkqualität (erkannte Sync-Words, verarbeitete Telegramme, verworfene Frames).
