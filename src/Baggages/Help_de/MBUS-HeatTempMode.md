### Temperaturen senden (Wärmemengenzähler)

Legt fest, welche Temperaturwerte auf den KNX-Bus übertragen werden:

- **Keine Temperaturen** – nur Energie, Durchfluss und Leistung werden gesendet
- **Vor- und Rücklauftemperatur** – Vorlauf (KO 4) und Rücklauf (KO 5)
- **Vorlauf und Temperaturdifferenz** – Vorlauf (KO 4) und Differenz VL−RL in K (KO 6)
- **Nur Temperaturdifferenz** – ausschließlich die Differenz VL−RL in K (KO 6)

Rücklauftemperatur und Temperaturdifferenz schließen sich gegenseitig aus, da viele Zähler nur eine der beiden Größen senden.

