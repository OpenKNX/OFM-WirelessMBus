### Dokumentation


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

