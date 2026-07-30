# Applikationsbeschreibung Wireless MBus

<!-- DOC HelpContext="Dokumentation" -->
## Wireless MBus

Das Modul empfängt Wireless M-Bus Telegramme (wMBus) von Zählern über ein CC1101-Funkmodul und sendet die Messwerte auf den KNX-Bus. Unterstützt werden Wasserzähler (warm und kalt) sowie Wärmemengenzähler.

Jeder Kanal ist über die eindeutige Meter-ID fest einem Zähler zugeordnet; eingehende Telegramme werden anhand dieser ID dem passenden Kanal zugewiesen. Verschlüsselte Telegramme werden bei hinterlegtem AES-Schlüssel automatisch entschlüsselt.

Pro Kanal werden je nach Zählertyp unterschiedliche Werte bereitgestellt:

- **Wasserzähler** – Verbrauch (m³), Durchfluss (m³/h) und die gemessene Wassertemperatur
- **Wärmemengenzähler** – zusätzlich Leistung (W) sowie wahlweise Vorlauf-/Rücklauftemperatur oder die Temperaturdifferenz zwischen beiden

Jeder Kanal hat außerdem ein eigenes Status-KO, das anzeigt, ob innerhalb der eingestellten Überwachungszeit (siehe Zählerüberwachung) ein gültiges Telegramm empfangen wurde.

Die Funkparameter (Empfangsmodus, Verstärkung, Bandbreite, Präambel-Qualitätsschwelle) gelten global für alle Kanäle und werden auf der Seite "Allgemein" eingestellt. Jeder Kanal wird individuell über die Kanalauswahl-Tabelle aktiviert und über seine Meter-ID einem physischen Zähler zugeordnet.

Über die Gerätekonsole lässt sich der Empfang live beobachten:

- **wmbus list** – zeigt alle bisher empfangenen Zähler mit ID, Hersteller, Typ, Verschlüsselungsstatus und Empfangsstärke (RSSI). Praktisch, um die Meter-ID eines Zählers ohne externen Scanner zu ermitteln.
- **wmbus debug** – schaltet eine fortlaufende Anzeige aller empfangenen Rohtelegramme ein bzw. aus.
- **wmbus stats** – zeigt Kennzahlen zur Funkqualität (erkannte Sync-Words, verarbeitete Telegramme, verworfene Frames).

<!-- DOC HelpContext="Mode" -->
### Empfangsmodus

Legt fest, auf welchem Funkprotokoll und in welchem Format der Empfänger lauscht:

- **Kombimodus (T1+C1B)** – Auto-Detect zwischen T1 (3of6-Kodierung) und C1B (raw bytes). Der Empfänger erkennt automatisch das Format anhand des Sync-Words und der Preamble.
- **T1** – 3of6-Kodierung
- **C1A** – Frame A Format, raw bytes
- **C1B** – Frame B Format, raw bytes
- **S1** – Manchester-Kodierung, längere Preamble

<!-- DOC HelpContext="Gain" -->
### Empfangsverstärkung (Gain)

Steuert den Vorverstärker (LNA) des CC1101:

- **Hoch** – Maximale Empfindlichkeit (Standardwert, für entfernte Zähler)
- **Mittel** – Reduzierte Verstärkung (−7 dB)
- **Niedrig** – Weitere Reduzierung (−15 dB)
- **Minimal** – Minimale Verstärkung (kurze Reichweite, störarme Umgebung)

<!-- DOC HelpContext="Bandwidth" -->
### Kanalbandbreite

Legt die Empfangsbandbreite des CC1101 fest:

- **Breit (325 kHz)** – Tolerant gegenüber Frequenzabweichungen (Standardwert)
- **Mittel (203 kHz)** – Ausgewogen
- **Schmal (135 kHz)** – Filtert Nachbarkanal-Störungen, erfordert stabile Frequenz

<!-- DOC HelpContext="Pqt" -->
### Präambel-Qualitätsschwelle (PQT)

Legt fest, wie viele gültige Preamble-Übergänge (0xAA-Bits) der CC1101 mindestens zählen muss, bevor er ein Sync-Word als gültig akzeptiert. Höhere Werte filtern Falschauslöser durch Rauschen, können aber echte Telegramme mit kurzer Preamble unterdrücken.

- **0** – Kein Filter (Standardwert): Jedes Sync-Word wird sofort akzeptiert
- **1–3** – Leichter Filter: Reduziert kurze Rauschimpulse
- **4–6** – Mittlerer Filter (empfohlen bei Störungen im 868-MHz-Band)
- **7** – Strenger Filter: Nur Frames mit langer, sauberer Preamble werden angenommen

<!-- DOC HelpContext="Zaehlertyp" -->
### Zählertyp

Wählt aus, welcher Messwert-Satz für diesen Kanal erwartet wird. Muss zuerst konfiguriert werden – alle weiteren Parameter und Kommunikationsobjekte sind nur bei aktivem Typ sichtbar.

- **Inaktiv** – Kanal ist deaktiviert, keine KNX-Objekte
- **Wasserzähler** – Volumen (m³), Durchfluss (m³/h), Vorlauftemperatur
- **Wärmemengenzähler** – Energie (kWh), Durchfluss (m³/h), Vorlauf-/Rücklauftemperatur, Leistung (W), Temperaturdifferenz (K)

<!-- DOC HelpContext="HeatTempMode" -->
### Temperaturen senden (Wärmemengenzähler)

Legt fest, welche Temperaturwerte auf den KNX-Bus übertragen werden:

- **Keine Temperaturen** – nur Energie, Durchfluss und Leistung werden gesendet
- **Vor- und Rücklauftemperatur** – Vorlauf (KO 4) und Rücklauf (KO 5)
- **Vorlauf und Temperaturdifferenz** – Vorlauf (KO 4) und Differenz VL−RL in K (KO 6)
- **Nur Temperaturdifferenz** – ausschließlich die Differenz VL−RL in K (KO 6)

Rücklauftemperatur und Temperaturdifferenz schließen sich gegenseitig aus, da viele Zähler nur eine der beiden Größen senden.

<!-- DOC HelpContext="MeterId" -->
### Meter-ID

Die eindeutige Gerätenummer des Zählers als **dezimale** Zahl. Die Meter-ID steht auf dem Zählergehäuse oder kann mit einem wMBus-Scanner ausgelesen werden. Sie wird verwendet, um eingehende Telegramme dem richtigen Kanal zuzuordnen.

<!-- DOC HelpContext="AesKey" -->
### AES-Schlüssel

Optionaler AES-128-Schlüssel für verschlüsselte Telegramme, einzugeben als **32 Hexadezimalzeichen** (z. B. `00112233445566778899AABBCCDDEEFF`). Wenn das Feld leer bleibt, wird keine Entschlüsselung versucht.

Das Status-KO zeigt an, ob die Entschlüsselung erfolgreich war.

<!-- DOC HelpContext="SendOnlyIfChanged" -->
### Nur senden bei Änderung

Wenn aktiv, wird ein Telegramm nur dann auf den KNX-Bus gesendet, wenn sich der Messwert gegenüber dem zuletzt gesendeten Wert tatsächlich geändert hat. Alle Messwerte werden unabhängig davon stets im KNX-Buffer aktualisiert, sodass Read-Requests jederzeit beantwortet werden.

<!-- DOC HelpContext="MinInterval" -->
### Mindestwartezeit

Legt fest, wie viel Zeit mindestens zwischen zwei gesendeten Telegrammen vergehen muss. Trifft ein neuer Messwert ein, bevor die Wartezeit abgelaufen ist, wird er zwar im Buffer gespeichert, aber nicht gesendet.

**Beispiel:** Ein Zähler sendet alle 15 Sekunden. Bei einer Mindestwartezeit von 1 Minute wird nur jedes vierte Telegramm auf den KNX-Bus weitergegeben.

Die Zeit besteht aus zwei Parametern: Zeitwert (Zahl) und Zeitbasis (Sekunden / Minuten).

<!-- DOC HelpContext="Watchdog" -->
### Zählerüberwachung

Legt fest, nach welcher Zeit ohne gültiges Telegramm das Status-KO auf **Inaktiv** wechselt.

Der Timer startet beim Gerätestart und wird bei jedem erfolgreich empfangenen und entschlüsselten Telegramm zurückgesetzt. Trifft innerhalb der eingestellten Zeit kein gültiges Telegramm ein – z. B. weil der Zähler außer Reichweite ist oder der Akku leer ist – wechselt der Status auf Inaktiv (0).

Ein AES-Entschlüsselungsfehler setzt den Status ebenfalls sofort auf Inaktiv, unabhängig vom Timer.

Transiente Fehler wie einzelne CRC-Fehler oder unvollständige Telegramme beeinflussen den Status nicht.
