### Mindestwartezeit

Legt fest, wie viel Zeit mindestens zwischen zwei gesendeten Telegrammen vergehen muss. Trifft ein neuer Messwert ein, bevor die Wartezeit abgelaufen ist, wird er zwar im Buffer gespeichert, aber nicht gesendet.

**Beispiel:** Ein Zähler sendet alle 15 Sekunden. Bei einer Mindestwartezeit von 1 Minute wird nur jedes vierte Telegramm auf den KNX-Bus weitergegeben.

Die Zeit besteht aus zwei Parametern: Zeitwert (Zahl) und Zeitbasis (Sekunden / Minuten).

