### Präambel-Qualitätsschwelle (PQT)

Legt fest, wie viele gültige Preamble-Übergänge (0xAA-Bits) der CC1101 mindestens zählen muss, bevor er ein Sync-Word als gültig akzeptiert. Höhere Werte filtern Falschauslöser durch Rauschen, können aber echte Telegramme mit kurzer Preamble unterdrücken.

- **0** – Kein Filter (Standardwert): Jedes Sync-Word wird sofort akzeptiert
- **1–3** – Leichter Filter: Reduziert kurze Rauschimpulse
- **4–6** – Mittlerer Filter (empfohlen bei Störungen im 868-MHz-Band)
- **7** – Strenger Filter: Nur Frames mit langer, sauberer Preamble werden angenommen

