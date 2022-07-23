# Pokémon Emerald

This is a [Universal Randomizer](https://github.com/Ajarmar/universal-pokemon-randomizer-zx)-compatible fork of [pret's Pokémon Emerald decompilation](https://github.com/pret/pokeemerald) with a number of qualify-of-life changes intended to improve the randomizer & nuzlocke experience.

To set up the repository, see [INSTALL.md](INSTALL.md).

## List of changes

* Player PC contains 99 rare candies instead of a potion
* Player is given 99 pokéballs instead of 5
* Rare candies and regular pokéballs can be purchased at most stores for ¥1
* Starters are shiny
* Gym leader & Elite Four member teams are shiny
* All evolution stones (including sun & moon) are available to purchase in Lavaridge
* TMs have unlimited uses
* Pokémon base stats updated to match Gen VIII
* Friendship and trade evolutions replaced with level & stone evolutions (see below)
* Evolution & learnset levels decreased (see below)
    * Starters normalized to evolve at 16 & 30
* HMs do not have to be taught to a pokémon in order to be used in the overworld (applies to cut, surf, strength, rock smash, waterfall, and dive)
* HM moves can be overwritten (except surf when outside of a battle, e.g. using a TM or learning a move from leveling up via rare candy)
* Fishing always succeeds and has a 15 second window to press A to start the encounter instead of ~0.5 seconds (makes fishing on emulator speedup significantly easier)
* Coin case is obtained in exchange for a pokéball instead of a harbor mail
* Game corner coin prices adjusted to make TMs cost ¥5,500 instead of ¥80,000
* When obtaining the pokénav, player is no longer held hostage until calling the president (menu can be exited immediately instead)
* Phone no longer rings randomly, unless the party's lead Pokémon has the lightning rod ability
* Moves
    * Constrict base power increased to 30
    * Rock smash base power increased to 40
* Pokémon adjustments
    * Ivysaur
        * Evolves into Venusaur at level ~~32~~ -> 30
        * Learns Razor Leaf at ~~22~~ -> 21
        * Learns Sweet Scent at ~~29~~ -> 27
        * Learns Growth at ~~38~~ -> 30
        * Learns Synthesis at ~~47~~ -> 38
        * Learns Solar Beam at ~~56~~ -> 44
    * Venusaur
        * Learns ~~Growth at 41~~ -> Synthesis at 36
        * Learns ~~Synthesis at 53~~ -> Solar Beam at 43
        * Learns ~~Solar Beam at 65~~ -> Growth at 50
    * Charmander
        * Learns Rage at ~~19~~ -> 16
    * Charmeleon
        * Evolves into Charizard at level ~~36~~ -> 30
        * Learns Scary Face at ~~27~~ -> 26
        * Learns Flamethrower at ~~34~~ -> 30
    * Charizard
        * Learns Slash at ~~44~~ -> 43
        * Learns Dragon Rage at ~~54~~ -> 50
        * Learns Fire Spin at ~~64~~ -> 57
    * Wartortle
        * Evolves into Blastoise at level ~~36~~ -> 30
        * Learns Protect at ~~31~~ -> 30
    * Blastoise
        * Learns Rain Dance at ~~42~~ -> 36
        * Learns Skull Bash at ~~55~~ -> 43
        * Learns Hydro Pump at ~~68~~ -> 50
    * Pidgey
        * Evolves into Pidgeotto at level ~~18~~ -> 16
        * Learns Whirlwind at ~~19~~ -> 16
    * Pidgeotto
        * Evolves into Pidgeot at level ~~36~~ -> 27
        * Learns Feather Dance at ~~34~~ -> 32
        * Learns Agility at ~~43~~ -> 38
        * Learns Mirror Move at ~~52~~ -> 45
    * Pidgeot
        * Learns Feather Dance at ~~34~~ -> 33
        * Learns Agility at ~~48~~ -> 39
        * Learns Mirror Move at ~~62~~ -> 45
    * Raticate
        * Learns Pursuit at ~~30~~ -> 26
        * Learns Super Fang at ~~40~~ -> 32
        * Learns Endeavor at ~~50~~ -> 38
    * Ekans
        * Evolves into Arbok at level ~~22~~ -> 20
    * Arbok
        * Learns Acid at ~~38~~ -> 34
        * Learns Stockpile at ~~46~~ -> 40
        * Learns Swallow at ~~46~~ -> 40
        * Learns Spit Up at ~~46~~ -> 40
        * Learns Haze at ~~56~~ -> 46
    * Sandshrew
        * Evolves into Sandslash at level ~~22~~ -> 20
    * Nidoran♀
        * Learns Poison Sting at ~~17~~ -> 16
    * Nidorina
        * Learns Fury Swipes at ~~34~~ -> 32
        * Learns Flatter at ~~43~~ -> 38
        * Learns Crunch at ~~53~~ -> 44
    * Nidoqueen
        * Learns ~~Tail Whip at 1~~ -> Body Slam at 32
    * Nidoran♂
        * Learns Poison Sting at ~~17~~ -> 16
    * Nidorino
        * Learns Fury Attack at ~~34~~ -> 32
        * Learns Flatter at ~~43~~ -> 38
        * Learns Horn Drill at ~~53~~ -> 44
    * Nidoking
        * Learns ~~Focus Energy at 1~~ -> Thrash at 32
    * Clefairy
        * Learns Double Slap at ~~13~~ -> 14
    * Zubat
        * Evolves into Golbat at level ~~22~~ -> 20
        * Learns Wing Attack at ~~21~~ -> 20
    * Golbat
        * Evolves into Crobat ~~with high friendship~~ -> at level 28
    * Oddish
        * Evolves into Gloom at level ~~21~~ -> 18
    * Gloom
        * Learns Moonlight at ~~35~~ -> 30
        * Learns Petal Dance at ~~44~~ -> 36
    * Paras
        * Evolves into Parasect at level ~~24~~ -> 20
    * Parasect
        * Learns Leech Life at ~~19~~ -> 20
    * Venonat
        * Evolves into Venomoth at level ~~31~~ -> 20
    * Venomoth
        * Learns Psychic at ~~52~~ -> 48
    * Diglett
        * Evolves into Dugtrio at level ~~26~~ -> 21
        * Learns Dig at ~~17~~ -> 13
        * Learns Mud Slap at ~~25~~ -> 20
        * Learns Slash at ~~33~~ -> 27
        * Learns Earthquake at ~~41~~ -> 34
        * Learns Fissure at ~~49~~ -> 41
    * Dugtrio
        * Learns Slash at ~~38~~ -> 32
        * Learns Earthquake at ~~51~~ -> 38
        * Learns Fissure at ~~64~~ -> 44
    * Meowth
        * Evolves into Persian at level ~~28~~ -> 20
    * Persian
        * Learns Faint Attack at ~~29~~ -> 26
        * Learns Screech at ~~38~~ -> 32
        * Learns Fury Swipes at ~~46~~ -> 38
        * Learns Slash at ~~53~~ -> 44
        * Learns Fake Out at ~~59~~ -> 50
    * Psyduck
        * Evolves into Golduck at level ~~33~~ -> 25
    * Golduck
        * Learns Psych Up at ~~31~~ -> 28
        * Learns Fury Swipes at ~~44~~ -> 34
        * Learns Hydro Pump at ~~58~~ -> 40
    * Mankey
        * Evolves into Primeape at level ~~28~~ -> 21
    * Arcanine
        * Learns ~~Roar at 1~~ -> Extreme Speed at 32
    * Poliwag
        * Evolves into Poliwhirl at level ~~25~~ -> 19
    * Poliwhirl
        * Evolves into Politoed ~~when traded with Kings Rock~~ -> at level 30
    * Kadabra
        * Evolves into Alakazam ~~when traded~~ -> at level 30
        * Learns Role Play at ~~33~~ -> 32
    * Machop
        * Evolves into Machoke at level ~~28~~ -> 20
    * Machoke
        * Evolves into Machamp ~~when traded~~ -> at level 30
        * Learns Vital Throw at ~~33~~ -> 30
        * Learns Submission at ~~41~~ -> 36
        * Learns Cross Chop at ~~46~~ -> 42
        * Learns Scary Face at ~~51~~ -> 48
        * Learns Dynamic Punch at ~~59~~ -> 54
    * Tentacool
        * Evolves into Tentacruel at level ~~30~~ -> 25
    * Tentacruel
        * Learns Barrier at ~~38~~ -> 36
        * Learns Screech at ~~47~~ -> 42
        * Learns Hydro Pump at ~~55~~ -> 48
    * Geodude
        * Evolves into Graveler at level ~~25~~ -> 20
        * Learns Self Destruct at ~~21~~ -> 20
    * Graveler
        * Evolves into Golem ~~when traded~~ -> at level 27
        * Learns Rollout at ~~29~~ -> 27
        * Learns Rock Blast at ~~37~~ -> 33
        * Learns Earthquake at ~~45~~ -> 39
        * Learns Explosion at ~~53~~ -> 45
        * Learns Double Edge at ~~62~~ -> 51
    * Ponyta
        * Evolves into Rapidash at level ~~40~~ -> 27
    * Rapidash
        * Learns Bounce at ~~50~~ -> 46
        * Learns Fire Blast at ~~63~~ -> 52
    * Slowpoke
        * Evolves into Slowbro at level ~~37~~ -> 27
        * Evolves into Slowking ~~when traded with Kings Rock~~ -> with Water Stone
        * Learns Disable at ~~29~~ -> 27
    * Magnemite
        * Evolves into Magneton at level ~~30~~ -> 26
    * Doduo
        * Evolves into Dodrio at level ~~31~~ -> 25
    * Seel
        * Evolves into Dewgong at level ~~34~~ -> 26
        * Learns Rest at ~~29~~ -> 26
        * Learns Take Down at ~~37~~ -> 32
        * Learns Ice Beam at ~~41~~ -> 37
        * Learns Safeguard at ~~49~~ -> 42
    * Grimer
        * Evolves into Muk at level ~~38~~ -> 26
    * Gastly
        * Evolves into Haunter at level ~~25~~ -> 21
    * Haunter
        * Evolves into Gengar ~~when traded~~ -> at level 30
        * Learns Confuse Ray at ~~31~~ -> 30
        * Learns Dream Eater at ~~39~~ -> 37
        * Learns Destiny Bond at ~~48~~ -> 44
    * Onix
        * Evolves into Steelix ~~when traded with Metal Coat~~ -> at level 27
    * Drowzee
        * Evolves into Hypno at level ~~26~~ -> 25
    * Krabby
        * Evolves into Kingler at level ~~28~~ -> 25
        * Learns Stomp at ~~27~~ -> 25
    * Voltorb
        * Evolves into Electrode at level ~~30~~ -> 26
        * Learns Self Destruct at ~~27~~ -> 26
    * Cubone
        * Evolves into Marowak at level ~~28~~ -> 20
        * Learns Focus Energy at ~~21~~ -> 20
    * Koffing
        * Evolves into Weezing at level ~~35~~ -> 26
    * Weezing
        * Learns Smokescreen at ~~25~~ -> 26
    * Rhyhorn
        * Evolves into Rhydon at level ~~42~~ -> 26
    * Chansey
        * Evolves into Blissey ~~with high friendship~~ -> at level 32
        * Learns Sing at ~~29~~ -> 28
        * Learns Egg Bomb at ~~35~~ -> 32
        * Learns Defense Curl at ~~41~~ -> 38
        * Learns Light Screen at ~~49~~ -> 44
        * Learns Double Edge at ~~57~~ -> 50
    * Horsea
        * Evolves into Seadra at level ~~32~~ -> 20
        * Learns Water Gun at ~~22~~ -> 20
    * Seadra
        * Evolves into Kingdra ~~when traded with Dragon Scale~~ -> at level 32
        * Learns Twister at ~~29~~ -> 27
        * Learns Agility at ~~40~~ -> 32
        * Learns Hydro Pump at ~~51~~ -> 38
        * Learns Dragon Dance at ~~62~~ -> 44
    * Goldeen
        * Evolves into Seaking at level ~~33~~ -> 20
        * Learns Flail at ~~24~~ -> 20
        * Learns Fury Attack at ~~29~~ -> 25
        * Learns Waterfall at ~~38~~ -> 30
        * Learns Horn Drill at ~~43~~ -> 35
        * Learns Agility at ~~52~~ -> 40
    * Seaking
        * Learns Waterfall at ~~41~~ -> 35
        * Learns Horn Drill at ~~49~~ -> 41
        * Learns Agility at ~~61~~ -> 47
    * Scyther
        * Evolves into Scizor ~~when traded with Metal Coat~~ -> at level 32
    * Magikarp
        * Learns Flail at ~~30~~ -> 20
    * Eevee
        * Evolves into Espeon with ~~high friendship during daytime~~ -> Sun Stone
        * Evolves into Umbreon with ~~high friendship during nighttime~~ -> Moon Stone
    * Porygon
        * Evolves into Porygon2 ~~when traded with Up Grade~~ -> at level 26
    * Omanyte
        * Evolves into Omastar at level ~~40~~ -> 25
    * Kabuto
        * Evolves into Kabutops at level ~~40~~ -> 25
    * Dratini
        * Evolves into Dragonair at level ~~30~~ -> 25
    * Dragonair
        * Evolves into Dragonite at level ~~55~~ -> 35
        * Learns Agility at ~~38~~ -> 35
    * Dragonite
        * Learns Agility at ~~38~~ -> 35
        * Learns Safeguard at ~~47~~ -> 41
        * Learns Wing Attack at ~~55~~ -> 47
        * Learns Outrage at ~~61~~ -> 53
        * Learns Hyper Beam at ~~75~~ -> 59
    * Bayleef
        * Evolves into Meganium at level ~~32~~ -> 30
        * Learns Synthesis at ~~23~~ -> 20
        * Learns Body Slam at ~~31~~ -> 25
        * Learns Light Screen at ~~39~~ -> 30
        * Learns Safeguard at ~~47~~ -> 36
        * Learns Solar Beam at ~~55~~ -> 42
    * Meganium
        * Learns Body Slam at ~~31~~ -> 30
        * Learns Light Screen at ~~41~~ -> 36
        * Learns Safeguard at ~~51~~ -> 42
        * Learns Solar Beam at ~~61~~ -> 48
    * Cyndaquil
        * Evolves into Quilava at level ~~14~~ -> 16
        * Learns Quick Attack at ~~19~~ -> 16
    * Quilava
        * Evolves into Typhlosion at level ~~36~~ -> 30
        * Learns Quick Attack at ~~21~~ -> 19
        * Learns Flame Wheel at ~~31~~ -> 25
        * Learns Swift at ~~42~~ -> 30
        * Learns Flamethrower at ~~54~~ -> 36
    * Typhlosion
        * Learns Flame Wheel at ~~31~~ -> 30
        * Learns Swift at ~~45~~ -> 36
        * Learns Flamethrower at ~~60~~ -> 42
    * Totodile
        * Evolves into Croconaw at level ~~18~~ -> 16
        * Learns Bite at ~~20~~ -> 16
    * Croconaw
        * Learns Scary Face at ~~28~~ -> 26
        * Learns Slash at ~~37~~ -> 30
        * Learns Screech at ~~45~~ -> 36
        * Learns Hydro Pump at ~~55~~ -> 42
    * Feraligatr
        * Learns Slash at ~~38~~ -> 36
        * Learns Screech at ~~47~~ -> 42
        * Learns Hydro Pump at ~~58~~ -> 48
    * Hoothoot
        * Learns Dream Eater at ~~48~~ -> 40
    * Noctowl
        * Learns Take Down at ~~33~~ -> 30
        * Learns Confusion at ~~41~~ -> 36
        * Learns Dream Eater at ~~57~~ -> 42
    * Ledyba
        * Learns Comet Punch at ~~15~~ -> 12
        * Learns Light Screen at ~~22~~ -> 18
        * Learns Reflect at ~~22~~ -> 18
        * Learns Safeguard at ~~22~~ -> 18
        * Learns Baton Pass at ~~29~~ -> 25
        * Learns Swift at ~~36~~ -> 31
        * Learns Agility at ~~43~~ -> 37
        * Learns Double Edge at ~~50~~ -> 43
    * Ledian
        * Learns Light Screen at ~~24~~ -> 20
        * Learns Reflect at ~~24~~ -> 20
        * Learns Safeguard at ~~24~~ -> 20
        * Learns Baton Pass at ~~33~~ -> 25
        * Learns Swift at ~~42~~ -> 30
        * Learns Agility at ~~51~~ -> 35
        * Learns Double Edge at ~~60~~ -> 40
    * Spinarak
        * Evolves into Ariados at level ~~22~~ -> 20
        * Learns Night Shade at ~~17~~ -> 16
        * Learns Leech Life at ~~23~~ -> 20
        * Learns Fury Swipes at ~~30~~ -> 25
        * Learns Spider Web at ~~37~~ -> 30
        * Learns Agility at ~~45~~ -> 35
        * Learns Psychic at ~~53~~ -> 40
    * Ariados
        * Learns Leech Life at ~~25~~ -> 22
        * Learns Fury Swipes at ~~34~~ -> 27
        * Learns Spider Web at ~~43~~ -> 32
        * Learns Agility at ~~53~~ -> 37
        * Learns Psychic at ~~63~~ -> 42
    * Chinchou
        * Evolves into Lanturn at level ~~27~~ -> 25
    * Pichu
        * Evolves into Pikachu ~~with high friendship~~ -> at level 14
    * Cleffa
        * Evolves into Clefairy ~~with high friendship~~ -> at level 14
    * Igglybuff
        * Evolves into Jigglypuff ~~with high friendship~~ -> at level 14
    * Togepi
        * Evolves into Togetic ~~with high friendship~~ -> at level 16
    * Natu
        * Learns Teleport at ~~20~~ -> 16
        * Learns Wish at ~~30~~ -> 22
        * Learns Future Sight at ~~30~~ -> 28
        * Learns Confuse Ray at ~~40~~ -> 34
        * Learns Psychic at ~~50~~ -> 40
    * Xatu
        * Learns Wish at ~~35~~ -> 26
        * Learns Future Sight at ~~35~~ -> 32
        * Learns Confuse Ray at ~~50~~ -> 38
        * Learns Psychic at ~~65~~ -> 44
    * Mareep
        * Learns Thunder Wave at ~~16~~ -> 15
    * Ampharos
        * Learns Light Screen at ~~42~~ -> 36
        * Learns Thunder at ~~57~~ -> 42
    * Azumarill
        * Learns Bubble Beam at ~~24~~ -> 22
        * Learns Double Edge at ~~34~~ -> 28
        * Learns Rain Dance at ~~45~~ -> 34
        * Learns Hydro Pump at ~~57~~ -> 40
    * Politoed
        * Learns Swagger at ~~51~~ -> 41
    * Skiploom
        * Learns Cotton Spore at ~~29~~ -> 27
        * Learns Mega Drain at ~~36~~ -> 33
    * Jumpluff
        * Learns Mega Drain at ~~44~~ -> 39
    * Wooper
        * Learns Amnesia at ~~21~~ -> 20
    * Quagsire
        * Learns Yawn at ~~35~~ -> 29
        * Learns Earthquake at ~~42~~ -> 35
        * Learns Rain Dance at ~~49~~ -> 41
        * Learns Mist at ~~61~~ -> 45
        * Learns Haze at ~~61~~ -> 45
    * Slowking
        * Learns Disable at ~~29~~ -> 27
        * Learns Headbutt at ~~34~~ -> 32
        * Learns Swagger at ~~43~~ -> 37
        * Learns Psychic at ~~48~~ -> 42
    * Pineco
        * Evolves into Forretress at level ~~31~~ -> 25
    * Forretress
        * Learns Explosion at ~~39~~ -> 35
        * Learns Spikes at ~~49~~ -> 41
        * Learns Double Edge at ~~59~~ -> 47
    * Steelix
        * Learns Rage at ~~25~~ -> 27
        * Learns Iron Tail at ~~45~~ -> 43
        * Learns Double Edge at ~~57~~ -> 55
    * Snubbull
        * Learns Roar at ~~26~~ -> 23
        * Learns Rage at ~~34~~ -> 29
        * Learns Take Down at ~~43~~ -> 35
        * Learns Crunch at ~~53~~ -> 41
    * Granbull
        * Learns Rage at ~~38~~ -> 34
        * Learns Take Down at ~~49~~ -> 40
        * Learns Crunch at ~~61~~ -> 46
    * Sneasel
        * Learns Faint Attack at ~~22~~ -> 20
        * Learns Fury Swipes at ~~29~~ -> 25
        * Learns Agility at ~~36~~ -> 30
        * Learns Icy Wind at ~~43~~ -> 35
        * Learns Slash at ~~50~~ -> 40
        * Learns Beat Up at ~~57~~ -> 45
        * Learns Metal Claw at ~~64~~ -> 50
    * Teddiursa
        * Learns Rest at ~~31~~ -> 30
    * Slugma
        * Evolves into Magcargo at level ~~38~~ -> 25
    * Magcargo
        * Learns Flamethrower at ~~36~~ -> 35
        * Learns Rock Slide at ~~48~~ -> 41
        * Learns Body Slam at ~~60~~ -> 47
    * Swinub
        * Evolves into Piloswine at level ~~33~~ -> 25
        * Learns Take Down at ~~28~~ -> 25
        * Learns Mist at ~~37~~ -> 32
        * Learns Blizzard at ~~46~~ -> 39
        * Learns Amnesia at ~~55~~ -> 46
    * Piloswine
        * Learns Mist at ~~42~~ -> 39
        * Learns Blizzard at ~~56~~ -> 45
        * Learns Amnesia at ~~70~~ -> 51
    * Octillery
        * Learns Focus Energy at ~~38~~ -> 32
        * Learns Ice Beam at ~~54~~ -> 40
        * Learns Hyper Beam at ~~70~~ -> 48
    * Houndour
        * Learns Bite at ~~25~~ -> 24
    * Houndoom
        * Learns Odor Sleuth at ~~35~~ -> 34
        * Learns Faint Attack at ~~43~~ -> 41
        * Learns Flamethrower at ~~51~~ -> 48
        * Learns Crunch at ~~59~~ -> 55
    * Kingdra
        * Learns Twister at ~~29~~ -> 32
        * Learns Agility at ~~40~~ -> 39
        * Learns Hydro Pump at ~~51~~ -> 46
        * Learns Dragon Dance at ~~62~~ -> 53
    * Porygon2
        * Learns Defense Curl at ~~24~~ -> 26
        * Learns Recycle at ~~44~~ -> 42
    * Smoochum
        * Evolves into Jynx at level ~~30~~ -> 20
        * Learns Confusion at ~~21~~ -> 19
    * Elekid
        * Evolves into Electabuzz at level ~~30~~ -> 25
        * Learns Swift at ~~25~~ -> 19
        * Learns Screech at ~~33~~ -> 25
        * Learns Thunderbolt at ~~41~~ -> 30
        * Learns Thunder at ~~49~~ -> 35
    * Magby
        * Evolves into Magmar at level ~~30~~ -> 25
        * Learns Fire Punch at ~~19~~ -> 17
        * Learns Smokescreen at ~~25~~ -> 19
        * Learns Sunny Day at ~~31~~ -> 25
        * Learns Flamethrower at ~~37~~ -> 30
        * Learns Confuse Ray at ~~43~~ -> 35
        * Learns Fire Blast at ~~49~~ -> 40
    * Larvitar
        * Evolves into Pupitar at level ~~30~~ -> 25
        * Learns Rock Slide at ~~22~~ -> 20
        * Learns Thrash at ~~29~~ -> 25
        * Learns Scary Face at ~~36~~ -> 30
        * Learns Crunch at ~~43~~ -> 35
        * Learns Earthquake at ~~50~~ -> 40
        * Learns Hyper Beam at ~~57~~ -> 45
    * Pupitar
        * Evolves into Tyranitar at level ~~55~~ -> 35
        * Learns Thrash at ~~29~~ -> 25
        * Learns Scary Face at ~~38~~ -> 30
        * Learns Crunch at ~~47~~ -> 35
        * Learns Earthquake at ~~56~~ -> 42
        * Learns Hyper Beam at ~~65~~ -> 49
    * Tyranitar
        * Learns Scary Face at ~~38~~ -> 36
        * Learns Crunch at ~~47~~ -> 42
        * Learns Earthquake at ~~61~~ -> 48
        * Learns Hyper Beam at ~~75~~ -> 54
    * Grovyle
        * Evolves into Sceptile at level ~~36~~ -> 30
    * Sceptile
        * Learns Slam at ~~43~~ -> 41
        * Learns Detect at ~~51~~ -> 47
        * Learns False Swipe at ~~59~~ -> 53
    * Combusken
        * Evolves into Blaziken at level ~~36~~ -> 30
        * Learns Bulk Up at ~~28~~ -> 26
        * Learns Quick Attack at ~~32~~ -> 30
    * Blaziken
        * Learns Mirror Move at ~~49~~ -> 48
        * Learns Sky Uppercut at ~~59~~ -> 54
    * Marshtomp
        * Evolves into Swampert at level ~~36~~ -> 30
        * Learns Take Down at ~~31~~ -> 30
    * Swampert
        * Learns Muddy Water at ~~39~~ -> 37
        * Learns Protect at ~~46~~ -> 43
        * Learns Earthquake at ~~52~~ -> 49
        * Learns Endeavor at ~~61~~ -> 55
    * Zigzagoon
        * Learns Mud Sport at ~~21~~ -> 20
    * Lombre
        * Learns Fake Out at ~~19~~ -> 18
        * Learns Fury Swipes at ~~25~~ -> 23
        * Learns Water Sport at ~~31~~ -> 28
        * Learns Thief at ~~37~~ -> 33
        * Learns Uproar at ~~43~~ -> 38
        * Learns Hydro Pump at ~~49~~ -> 43
    * Nuzleaf
        * Learns Fake Out at ~~19~~ -> 18
        * Learns Torment at ~~25~~ -> 23
        * Learns Faint Attack at ~~31~~ -> 28
        * Learns Razor Wind at ~~37~~ -> 33
        * Learns Swagger at ~~43~~ -> 38
        * Learns Extrasensory at ~~49~~ -> 43
    * Taillow
        * Learns Endeavor at ~~26~~ -> 25
    * Swellow
        * Learns Endeavor at ~~28~~ -> 25
        * Learns Aerial Ace at ~~38~~ -> 32
        * Learns Agility at ~~49~~ -> 39
    * Pelipper
        * Learns Stockpile at ~~33~~ -> 30
        * Learns Swallow at ~~33~~ -> 30
        * Learns Spit Up at ~~47~~ -> 36
        * Learns Hydro Pump at ~~61~~ -> 42
    * Ralts
        * Evolves into Kirlia at level ~~20~~ -> 16
    * Kirlia
        * Learns Imprison at ~~33~~ -> 30
        * Learns Future Sight at ~~40~~ -> 35
        * Learns Hypnosis at ~~47~~ -> 40
        * Learns Dream Eater at ~~54~~ -> 45
    * Gardevoir
        * Learns Future Sight at ~~42~~ -> 39
        * Learns Hypnosis at ~~51~~ -> 45
        * Learns Dream Eater at ~~60~~ -> 50
    * Masquerain
        * Learns Scary Face at ~~33~~ -> 31
        * Learns Stun Spore at ~~40~~ -> 36
        * Learns Silver Wind at ~~47~~ -> 41
        * Learns Whirlwind at ~~53~~ -> 46
    * Shroomish
        * Learns Growth at ~~36~~ -> 34
        * Learns Giga Drain at ~~45~~ -> 40
        * Learns Spore at ~~54~~ -> 46
    * Breloom
        * Learns Sky Uppercut at ~~36~~ -> 34
        * Learns Mind Reader at ~~45~~ -> 40
        * Learns Dynamic Punch at ~~54~~ -> 46
    * Vigoroth
        * Evolves into Slaking at level ~~36~~ -> 32
    * Shedinja
        * Learns Mind Reader at ~~19~~ -> 20
        * Learns Shadow Ball at ~~38~~ -> 37
        * Learns Grudge at ~~45~~ -> 43
    * Whismur
        * Evolves into Loudred at level ~~20~~ -> 17
    * Loudred
        * Evolves into Exploud at level ~~40~~ -> 27
        * Learns Supersonic at ~~23~~ -> 22
        * Learns Stomp at ~~29~~ -> 27
        * Learns Screech at ~~37~~ -> 31
        * Learns Roar at ~~43~~ -> 36
        * Learns Rest at ~~51~~ -> 41
        * Learns Sleep Talk at ~~51~~ -> 46
        * Learns Hyper Voice at ~~57~~ -> 50
    * Exploud
        * Learns Stomp at ~~29~~ -> 27
        * Learns Screech at ~~37~~ -> 32
        * Learns Hyper Beam at ~~40~~ -> 37
        * Learns Roar at ~~45~~ -> 40
        * Learns Rest at ~~55~~ -> 45
        * Learns Sleep Talk at ~~55~~ -> 45
        * Learns Hyper Voice at ~~63~~ -> 50
    * Hariyama
        * Learns Belly Drum at ~~40~~ -> 39
        * Learns Seismic Toss at ~~51~~ -> 50
    * Azurill
        * Evolves into Marill ~~with high friendship~~ -> at level 14
        * Learns Slam at ~~15~~ -> 14
    * Aron
        * Evolves into Lairon at level ~~32~~ -> 20
        * Learns Roar at ~~21~~ -> 20
    * Lairon
        * Evolves into Aggron at level ~~42~~ -> 32
    * Aggron
        * Learns Protect at ~~37~~ -> 35
        * Learns Metal Sound at ~~50~~ -> 42
        * Learns Double Edge at ~~63~~ -> 50
    * Meditite
        * Evolves into Medicham at level ~~37~~ -> 25
        * Learns Calm Mind at ~~28~~ -> 25
    * Medicham
        * Learns Psych Up at ~~40~~ -> 39
        * Learns Reversal at ~~46~~ -> 45
        * Learns Recover at ~~54~~ -> 50
    * Electrike
        * Evolves into Manectric at level ~~26~~ -> 25
    * Manectric
        * Learns Bite at ~~39~~ -> 37
        * Learns Thunder at ~~45~~ -> 43
        * Learns Charge at ~~53~~ -> 49
    * Gulpin
        * Evolves into Swalot at level ~~26~~ -> 25
    * Carvanha
        * Evolves into Sharpedo at level ~~30~~ -> 25
        * Learns Screech at ~~28~~ -> 25
    * Wailmer
        * Evolves into Wailord at level ~~40~~ -> 30
        * Learns Water Pulse at ~~28~~ -> 27
        * Learns Mist at ~~32~~ -> 30
        * Learns Rest at ~~37~~ -> 35
    * Wailord
        * Learns Water Spout at ~~44~~ -> 43
        * Learns Amnesia at ~~52~~ -> 49
        * Learns Hydro Pump at ~~59~~ -> 55
    * Numel
        * Evolves into Camerupt at level ~~33~~ -> 25
    * Camerupt
        * Learns Eruption at ~~45~~ -> 44
        * Learns Fissure at ~~55~~ -> 50
    * Spoink
        * Evolves into Grumpig at level ~~32~~ -> 27
        * Learns Confuse Ray at ~~25~~ -> 24
        * Learns Magic Coat at ~~28~~ -> 27
    * Grumpig
        * Learns Psychic at ~~37~~ -> 34
        * Learns Rest at ~~43~~ -> 40
        * Learns Snore at ~~43~~ -> 40
        * Learns Bounce at ~~55~~ -> 46
    * Trapinch
        * Evolves into Vibrava at level ~~35~~ -> 25
    * Vibrava
        * Evolves into Flygon at level ~~45~~ -> 35
        * Learns Crunch at ~~33~~ -> 30
    * Flygon
        * Learns Sandstorm at ~~53~~ -> 47
        * Learns Hyper Beam at ~~65~~ -> 53
    * Cacnea
        * Evolves into Cacturne at level ~~32~~ -> 25
    * Swablu
        * Evolves into Altaria at level ~~35~~ -> 27
    * Barboach
        * Evolves into Whiscash at level ~~30~~ -> 25
        * Learns Amnesia at ~~21~~ -> 20
        * Learns Rest at ~~26~~ -> 25
        * Learns Snore at ~~26~~ -> 25
    * Whiscash
        * Learns Earthquake at ~~36~~ -> 32
        * Learns Future Sight at ~~46~~ -> 38
        * Learns Fissure at ~~56~~ -> 44
    * Corphish
        * Evolves into Crawdaunt at level ~~30~~ -> 25
        * Learns Knock Off at ~~26~~ -> 25
    * Crawdaunt
        * Learns Taunt at ~~34~~ -> 33
        * Learns Guillotine at ~~52~~ -> 50
    * Baltoy
        * Evolves into Claydol at level ~~36~~ -> 30
        * Learns Sandstorm at ~~31~~ -> 30
    * Claydol
        * Learns Explosion at ~~55~~ -> 50
    * Lileep
        * Evolves into Cradily at level ~~40~~ -> 30
    * Cradily
        * Learns Confuse Ray at ~~29~~ -> 30
        * Learns Ancient Power at ~~48~~ -> 43
        * Learns Stockpile at ~~60~~ -> 50
        * Learns Spit Up at ~~60~~ -> 50
        * Learns Swallow at ~~60~~ -> 50
    * Anorith
        * Evolves into Armaldo at level ~~40~~ -> 30
        * Learns Protect at ~~31~~ -> 30
    * Armaldo
        * Learns Protect at ~~31~~ -> 30
        * Learns Ancient Power at ~~37~~ -> 35
        * Learns Fury Cutter at ~~46~~ -> 40
        * Learns Slash at ~~55~~ -> 45
        * Learns Rock Blast at ~~64~~ -> 50
    * Feebas
        * Evolves into Milotic ~~with high beauty~~ -> at level 20
        * Learns Flail at ~~30~~ -> 20
    * Kecleon
        * Learns Substitute at ~~40~~ -> 37
        * Learns Ancient Power at ~~49~~ -> 44
    * Shuppet
        * Evolves into Banette at level ~~37~~ -> 30
        * Learns Will O Wisp at ~~32~~ -> 30
    * Banette
        * Learns Will O Wisp at ~~32~~ -> 30
        * Learns Faint Attack at ~~39~~ -> 35
        * Learns Shadow Ball at ~~48~~ -> 40
        * Learns Snatch at ~~55~~ -> 45
        * Learns Grudge at ~~64~~ -> 50
    * Duskull
        * Evolves into Dusclops at level ~~37~~ -> 25
        * Learns Pursuit at ~~27~~ -> 25
    * Dusclops
        * Learns Curse at ~~34~~ -> 32
        * Learns Mean Look at ~~51~~ -> 45
        * Learns Future Sight at ~~58~~ -> 50
    * Snorunt
        * Evolves into Glalie at level ~~42~~ -> 28
    * Glalie
        * Learns Hail at ~~42~~ -> 40
        * Learns Blizzard at ~~53~~ -> 45
        * Learns Sheer Cold at ~~61~~ -> 50
    * Spheal
        * Evolves into Sealeo at level ~~32~~ -> 25
    * Sealeo
        * Evolves into Walrein at level ~~44~~ -> 32
        * Learns Rest at ~~39~~ -> 35
    * Walrein
        * Learns Rest at ~~39~~ -> 37
        * Learns Snore at ~~39~~ -> 37
        * Learns Blizzard at ~~50~~ -> 43
        * Learns Sheer Cold at ~~61~~ -> 50
    * Clamperl
        * Evolves into Huntail ~~when traded with Deep Sea Tooth~~ -> at level 25
        * Evolves into Gorebyss ~~when traded with Deep Sea Scale~~ -> with Water Stone
    * Huntail
        * Learns Water Pulse at ~~22~~ -> 25
        * Learns Crunch at ~~36~~ -> 35
        * Learns Baton Pass at ~~43~~ -> 40
        * Learns Hydro Pump at ~~50~~ -> 45
    * Gorebyss
        * Learns Water Pulse at ~~22~~ -> 25
        * Learns Psychic at ~~36~~ -> 35
        * Learns Baton Pass at ~~43~~ -> 40
        * Learns Hydro Pump at ~~50~~ -> 45
    * Relicanth
        * Learns Rock Tomb at ~~15~~ -> 13
        * Learns Yawn at ~~22~~ -> 18
        * Learns Take Down at ~~29~~ -> 23
        * Learns Mud Sport at ~~36~~ -> 28
        * Learns Ancient Power at ~~43~~ -> 33
        * Learns Rest at ~~50~~ -> 38
        * Learns Double Edge at ~~57~~ -> 44
        * Learns Hydro Pump at ~~64~~ -> 50
    * Bagon
        * Evolves into Shelgon at level ~~30~~ -> 25
    * Shelgon
        * Evolves into Salamence at level ~~50~~ -> 35
        * Learns Dragon Breath at ~~38~~ -> 35
    * Salamence
        * Learns Dragon Breath at ~~38~~ -> 33
        * Learns Scary Face at ~~47~~ -> 35
        * Learns Fly at ~~50~~ -> 40
        * Learns Crunch at ~~61~~ -> 45
        * Learns Dragon Claw at ~~79~~ -> 50
        * Learns Double Edge at ~~93~~ -> 55
    * Metang
        * Evolves into Metagross at level ~~45~~ -> 35
        * Learns Psychic at ~~38~~ -> 35
    * Metagross
        * Learns Meteor Mash at ~~55~~ -> 50
        * Learns Agility at ~~66~~ -> 55
        * Learns Hyper Beam at ~~77~~ -> 60
