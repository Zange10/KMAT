# Itinerary Calculator:
## Passes if:
- dv-requirements are met with each itinerary
- date requirements are met with each itinerary
- fly-by body_names requirements are met
- has only valid itineraries (Periapsides etc.)
## Tests:
### Stock:
- 1: Kerbin -> Eeloo; 1-001 | 2-001 | 3000; dv_dep = 1500 | Bodies: all 
- 2: Kerbin -> Eeloo; 1-001 | 2-001 | 3000; dv_dep = 1800, dv_arr = 1800 (circ) | Bodies: Eve, Kerbin, Duna, Eeloo
- 3: Kerbin -> Kerbin; 1-001 | 2-001 | 1000; dv_dep = 1800 | Bodies: Eve, Kerbin, Duna, Jool
### RSS (Ephem)
- 4: Earth -> Earth; 1959-01-01 | 1960-01-01 | 1000; dv_dep = 5000 | Bodies: all
- 5: Earth -> Earth; 1959-01-01 | 1960-01-01 | 1000; dv_dep = 5000 | Bodies: Mercury, Venus, Earth
- 6: Earth -> Jupiter; 1967-01-01 | 1968-01-01 | 3000; dv_dep = 4500 | Bodies: Venus, Earth, Mars, Jupiter

# Sequence Calculator:
## Passes if:
- dv-requirements are met with each itinerary
- date requiremetns are met with each itinerary
- each itinerary is in defined sequence
- has only valid itineraries (Periapsides etc.)
### Stock:
- 1: Kerbin -> Eve; 2-001 | 3-001 | 300; dv_dep = 1200; dv_arr = 1500 (circ)
- 2: Kerbin -> Eve -> Duna -> Kerbin; 2-001 | 3-001 | 1000; dv_dep = 1500
### RSS (Ephem)
- 3: Earth -> Mars; 1950-01-01 | 1950-06-01 | 1950-11-01 | 600; dv_dep = 5000; dv_arr = 4000 (circ); dv_tot = 8200
- 4: Earth -> Jupiter -> Saturn -> Uranus -> Neptune; 1977-01-01 | 1978-01-01 | 8000; dv_dep = 7200

# Porkchop Analyzer:
## Passes if:
- has only valid itineraries (Periapsides etc.)
## Tests:
- Load number of different .itins files

# Transfer Planner:
## Passes if:
- itinerary is valid (Periapsides etc.)
## Tests:
- Load number of different .itin files