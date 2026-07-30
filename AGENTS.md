# AGENTS für OFM-WirelessMBus

## Ziel

Dieses Modul empfängt Wireless M-Bus Telegramme (Wasser- und Wärmemengenzähler) über ein CC1101-Funkmodul und sendet die Messwerte auf den KNX-Bus.

## Prefix und Namenskonventionen

- **Modulprefix**: `MBUS`
- **C++-Klassen**: `MBusModule`, `MBusChannel` (Präfix `MBus` für spätere Erweiterbarkeit auf kabelgebundenen M-Bus)
- **Kein** `WMB`-Prefix irgendwo verwenden — weder in C++ noch in XML noch in Doku
- **ETS-Anzeigename**: `WirelessMBUS`, **Icon**: `gauge`

## Versionierungs-Schema (XML)

| Platzhalter | Bedeutung |
|-------------|-----------|
| `%TT%` | Modultyp (zweistellig), analog OFM-SML |
| `%CC%` | Kanalnummer (zweistellig), analog OFM-SML |

Parameter-IDs: `%AID%_UP-%TT%%CC%NNN` bzw. `%AID%_P-%TT%%CC%NNN` (NNN = dreistellige Nummer).
**Nicht** das NetworkBridge-Schema `%T%%CCC%` verwenden.

## Hardware-Pins

Die CC1101-Pins werden **nicht** im OFM hartkodiert. Das einbindende Nutzerprojekt definiert:

| Makro | Bedeutung |
|-------|-----------|
| `WMBUS_CC1101_CS` | GPIO Chip-Select |
| `WMBUS_CC1101_GDO0` | GPIO Interrupt |
| `WMBUS_SPI` | SPI-Instanz (z.B. `SPI`, `SPI1`) |
| `WMBUS_SPI_MISO` | Optional: MISO-Pin |
| `WMBUS_SPI_MOSI` | Optional: MOSI-Pin |
| `WMBUS_SPI_SCK` | Optional: SCK-Pin |

Wenn `WMBUS_SPI_MISO` definiert ist, ruft `MBusModule::setup()` vor `_radio.begin()` den Aufruf `WMBUS_SPI.begin(WMBUS_SPI_SCK, WMBUS_SPI_MISO, WMBUS_SPI_MOSI)` auf.

## Kanalanzahl

Die Kanalanzahl wird durch das OAM über `MBUS_ChannelCount` vorgegeben. Das XML-Schema (`%TT%%CC%`) erlaubt bis zu 99 Kanäle. Im OFM selbst wird keine feste Grenze kodiert.

## Zählertypen

Typ 0 = Inaktiv: Kanal wird übersprungen, keine Parameter, keine KOs aktiv.
Typ 1 = Wasserzähler: sendet Volumen (m³), Durchfluss (m³/h), Vorlauftemperatur.
Typ 2 = Wärmemengenzähler: sendet Energie (kWh), Volumen (m³), Vor- und Rücklauftemperatur.

KO-Slots werden typ-übergreifend wiederverwendet (spart KOs):
- KO 0: Status (DPT_StatusGen)
- KO 1: Hauptmesswert (Volumen m³ oder Energie kWh)
- KO 2: Zweitmesswert (Durchfluss m³/h oder Volumen m³)
- KO 3: Vorlauftemperatur
- KO 4: Rücklauftemperatur (nur Typ 2)

## Regeln für Weiterentwicklung

1. Neue Zählertypen bekommen einen eigenen `case`-Zweig in `MBusChannel::processFrame()`.
2. Keine Typunterscheidung außerhalb von `MBusChannel`.
3. `MBusModule` verwaltet nur Radio, Receiver und das Channel-Array.
4. AES-Keys werden einmalig in `MBusModule::setup()` für alle konfigurierten Kanäle via `_wmbus.setKey()` registriert.
5. Frame-Routing erfolgt ausschließlich durch Vergleich `frame.meterId()` mit `_channels[i]->meterId()`.

## AES-Key

Der AES-Key wird direkt im ETS-Feld gespeichert (`SizeInBit="256"` = 32 Hex-Zeichen).
Leeres Feld (Länge 0 oder erstes Byte = 0) bedeutet kein AES.

## Meter-ID

Die Meter-ID ist eine dezimale `uint32_t`-Zahl (kein Hex-Eingabefeld in ETS).
Intern wird sie mit `wmbus.setKey(meterId, keyBytes)` und für das Frame-Routing genutzt.

## Dokumentation und Hilfe

- Dokumentation liegt in `doc/Applikationsbeschreibung-WirelessMBus.md`
- Jede sichtbare `ParameterRefRef` bekommt einen `HelpContext` mit Prefix `MBUS-`
- Jede verwendete `HelpContext`-Id muss in der Applikationsbeschreibung als `<!-- DOC HelpContext="MBUS-..." -->` dokumentiert sein
- Baggages werden über VS Code Task "OpenKNXproducer Documentation" erzeugt (`.vscode/tasks.json`)
- Deutsche Texte mit echten Umlauten (ä, ö, ü, ß) schreiben
