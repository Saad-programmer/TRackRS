# Cultural review checklist: M2 (Zagora Prologue vertical slice)

```
gate: m2
reviewed: false   # set true only after the maintainer confirms every item
reviewer: (maintainer)
```

Amendment §A1 ("Respect") and §A10. Claude prepared this list. Every item
is a claim for a human to check against the M2 evidence images in
[PHASE_M2.md](../reports/PHASE_M2.md). None of it is self-certified.

| # | Item | What M2 contains | Status |
|---|---|---|---|
| 1 | No real brands, sponsors, teams or people | The event "Rallye Sahara Draâ" is fictional. The cars "Draa Raid T1" and "Erg Lite T3" are fictional, with invented figures. Liveries are plain colour schemes with no logos. The marque field reads "Rallye Sahara Draa (fictional)". | to confirm |
| 2 | No real event or organiser branding | No ASO, no "Dakar", no real rally name or logo anywhere. The start arch is plain painted steel. | to confirm |
| 3 | Bilingual signage | Six fictional event signs in French and Arabic (`textures/signs/signs.json`): event name, departure, caution (race), Zagora 4 km, finish, spectator zone. Arabic is shaped with raqm in Noto Naskh Arabic. **Every Arabic line is `verified:false`**: a fluent reader must check spelling and register. Digits are Latin (A10). | **needs a fluent reviewer** |
| 4 | Signs do not imitate official road signage | Event-style panels (cream, terracotta border), not Moroccan road-sign designs. | to confirm |
| 5 | Place and landscape | Real place name (Zagora) used for geography only. Terrain is SRTM-derived (`derived`), with elevation above 740 m compressed. Route, palmeraie, ksar and village layout are invented, not a replica of any real ksar or street. | to confirm |
| 6 | Architecture and people | Same generic kit as M1: rammed-earth ksar, mud and concrete houses, khaima. Spectators are the existing generic crowd; local dress is Phase 5. No caricature. | to confirm |
| 7 | Audio | None added. No religious recitation or call to prayer. | n/a in M2 |
| 8 | Animals | Camels near a khaima and well, goats near the village, never on the road. | to confirm |
