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
* EV reducing berries can be purchased in Lilycove instead of X items
* Friendship and trade evolutions replaced with level & stone evolutions (see below)
* Evolution & learnset levels decreased (see below)
    * Starters normalized to evolve at 16 & 32
* HMs do not have to be taught to a pokémon in order to be used in the overworld (applies to cut, surf, strength, rock smash, waterfall, and dive)
* HM moves can be overwritten
* Fishing always succeeds and has a 15 second window to press A to start the encounter instead of ~0.5 seconds (makes fishing on emulator speedup significantly easier)
* Coin case is obtained in exchange for a pokéball instead of a harbor mail
* Game corner coin prices adjusted to make TMs cost ¥5,500 instead of ¥80,000
* When obtaining the pokénav, player is no longer held hostage until calling the president (menu can be exited immediately instead)
* Phone no longer rings randomly, unless the party's lead Pokémon has the lightning rod ability
* Pacifidlog TM guy doesn't require a 7 day wait and gives TM21 Frustration for showing him a pokemon with <150 friendship instead of <50
* Moves
    * Constrict base power increased to 30
    * Rock smash base power increased to 40
* Pokémon adjustments
    * Ivysaur
        * Learns sweet scent at ~~29~~ -> 27
        * Learns growth at ~~38~~ -> 31
    * Charmander
        * Learns rage at ~~19~~ -> 16
    * Charmeleon
        * Evolves at ~~36~~ -> 32
        * Learns scary face at ~~27~~ -> 26
        * Learns flamethrower at ~~34~~ -> 32
    * Wartortle
        * Evolves at ~~36~~ -> 32
    * Pidgey
        * Evolves at ~~18~~ -> 16
        * Learns whirlwind at ~~19~~ -> 16
    * Pidgeotto
        * Evolves at ~~36~~ -> 32
        * Learns feather dance at ~~34~~ -> 32
        * Learns agility at ~~43~~ -> 38
        * Learns mirror move at ~~52~~ -> 45
    * Ekans
        * Evolves at ~~22~~ -> 16
    * Zubat
        * Evolves at ~~22~~ -> 16
    * Golbat
        * Evolves at ~~high friendship~~ -> 22
    * Oddish
        * Evolves at ~~21~~ -> 16
        * Learns poison powder at ~~14~~ -> 12
        * Learns stun spore at ~~16~~ -> 14
        * Learns sleep powder at ~~18~~ -> 16
    * Paras
        * Evolves at ~~24~~ -> 20
    * Venonat
        * Evolves at ~~31~~ -> 20
    * Diglett
        * Evolves at ~~26~~ -> 21
        * Learns dig at ~~17~~ -> 13
        * Learns mud slap at ~~25~~ -> 20
        * Learns slash at ~~33~~ -> 27
        * Learns earthquake at ~~41~~ -> 34
        * Learns fissure at ~~49~~ -> 41
    * Meowth
        * Evolves at ~~28~~ -> 20
    * Psyduck
        * Evolves at ~~33~~ -> 28
        * Learns psych up at ~~31~~ -> 28
        * Learns fury swipes at ~~40~~ -> 35
        * Learns hydro pump at ~~50~~ -> 43
    * Mankey
        * Evolves at ~~28~~ -> 25
        * Learns focus energy at ~~27~~ -> 25
    * Poliwag
        * Evolves at ~~25~~ -> 20
    * Kadabra
        * Evolves at ~~trade~~ -> 32
        * Learns role play at ~~33~~ -> 32
    * Machop
        * Evolves at ~~28~~ -> 16
        * Learns revenge at ~~25~~ -> 24
    * Machoke
        * Evolves at ~~trade~~ -> 32
        * Learns vital throw at ~~33~~ -> 32
    * Tentacool
        * Evolves at ~~30~~ -> 27
        * Learns wrap at ~~30~~ -> 27
    * Geodude
        * Evolves at ~~25~~ -> 21
        * Learns rollout at ~~26~~ -> 25
    * Graveler
        * Evolves at ~~trade~~ -> 32
    * Ponyta
        * Evolves at ~~40~~ -> 28
    * Slowpoke
        * Evolves into Slowbro at ~~37~~ -> 28
        * Evolves into Slowking with ~~kings rock trade~~ -> water stone
        * Learns disable at ~~29~~ -> 28
    * Magnemite
        * Evolves at ~~30~~ -> 26
    * Doduo
        * Evolves at ~~31~~ -> 26
    * Seel
        * Evolves at ~~34~~ -> 26
        * Learns rest at ~~29~~ -> 26
        * Learns take down at ~~37~~ -> 32
        * Learns ice beam at ~~41~~ -> 37
        * Learns safeguard at ~~49~~ -> 42
    * Grimer
        * Evolves at ~~38~~ -> 26
    * Gastly
        * Evolves at ~~25~~ -> 21
    * Haunter
        * Evolves at ~~trade~~ -> 32
    * Onix
        * Evolves at ~~metal coat trade~~ -> 25
    * Krabby
        * Evolves at ~~28~~ -> 26
        * Learns stomp at ~~27~~ -> 26
    * Voltorb
        * Evolves at ~~30~~ -> 26
        * Learns self destruct at ~~27~~ -> 26
    * Cubone
        * Evolves at ~~28~~ -> 26
    * Koffing
        * Evolves at ~~35~~ -> 26
    * Rhyhorn
        * Evolves at ~~42~~ -> 26
    * Chansey
        * Evolves at ~~high friendship~~ -> 25
    * Horsea
        * Evolves at ~~32~~ -> 16
    * Seadra
        * Evolves at ~~dragon scale trade~~ -> 32
    * Goldeen
        * Evolves at ~~33~~ -> 16
    * Scyther
        * Evolves at ~~metal coat trade~~ -> 32
    * Magikarp
        * Learns flail at ~~30~~ -> 20
    * Eevee
        * Evolves into Espeon with ~~high friendship during daytime~~ -> sun stone
        * Evolves into Umbreon with ~~high friendship during nighttime~~ -> moon stone
    * Porygon
        * Evolves at ~~upgrade trade~~ -> 25
    * Omanyte
        * Evolves at ~~40~~ -> 25
    * Kabuto
        * Evolves at ~~40~~ -> 25
    * Dratini
        * Evolves at ~~30~~ -> 26
    * Dragonair
        * Evolves at ~~55~~ -> 35
        * Learns agility at ~~38~~ -> 35
    * Cyndaquil
        * Evolves at ~~14~~ -> 16
        * Learns quick attack at ~~19~~ -> 16
    * Quilava
        * Evolves at ~~36~~ -> 32
        * Learns quick attack at ~~21~~ -> 19
        * Learns flame wheel at ~~31~~ -> 25
        * Learns swift at ~~42~~ -> 31
        * Learns flamethrower at ~~54~~ -> 42
    * Totodile
        * Evolves at ~~18~~ -> 16
        * Learns bite at ~~20~~ -> 16
    * Croconaw
        * Evolves at ~~30~~ -> 32
        * Learns scary face at ~~28~~ -> 26
        * Learns slash at ~~37~~ -> 32
    * Chinchou
        * Evolves at ~~27~~ -> 25
    * Pichu
        * Evolves at ~~high friendship~~ -> 14
    * Cleffa
        * Evolves at ~~high friendship~~ -> 14
    * Igglybuff
        * Evolves at ~~high friendship~~ -> 14
    * Togepi
        * Evolves at ~~high friendship~~ -> 16
    * Pineco
        * Evolves at ~~31~~ -> 25
    * Teddiursa
        * Learns rest at ~~31~~ -> 30
    * Slugma
        * Evolves at ~~38~~ -> 25
    * Swinub
        * Evolves at ~~33~~ -> 25
        * Learns take down at ~~28~~ -> 25
        * Learns mist at ~~37~~ -> 32
        * Learns blizzard at ~~46~~ -> 39
        * Learns amnesia at ~~55~~ -> 46
    * Smoochum
        * Evolves at ~~30~~ -> 19
        * Learns confusion at ~~21~~ -> 19
    * Elekid
        * Evolves at ~~30~~ -> 19
        * Learns swift at ~~25~~ -> 19
        * Learns screech at ~~33~~ -> 25
        * Learns thunderbolt at ~~41~~ -> 30
        * Learns thunder at ~~49~~ -> 35
    * Magby
        * Evolves at ~~30~~ -> 19
        * Learns fire punch at ~~19~~ -> 17
        * Learns smokescreen at ~~25~~ -> 19
        * Learns sunny day at ~~31~~ -> 25
        * Learns flamethrower at ~~37~~ -> 30
        * Learns confuse ray at ~~43~~ -> 35
        * Learns fire blast at ~~49~~ -> 40
    * Larvitar
        * Evolves at ~~30~~ -> 25
    * Pupitar
        * Evolves at ~~55~~ -> 35
        * Learns scary face at ~~38~~ -> 35
    * Grovyle
        * Evolves at ~~36~~ -> 32
    * Combusken
        * Evolves at ~~36~~ -> 32
    * Marshtomp
        * Evolves at ~~36~~ -> 32
    * Wailmer
        * Evolves at ~~40~~ -> 30
    * Baltoy
        * Evolves at ~~36~~ -> 30
        * Learns sandstorm at ~~31~~ -> 30
    * Barboach
        * Evolves at ~~30~~ -> 25
    * Corphish
        * Evolves at ~~30~~ -> 25
        * Learns knock off at ~~26~~ -> 25
    * Feebas
        * evolves at ~~high beauty~~ -> 20
        * Learns flail at ~~30~~ -> 20
    * Carvanha
        * Evolves at ~~30~~ -> 25
        * Learns screech at ~~28~~ -> 25
    * Trapinch
        * Evolves at ~~35~~ -> 25
    * Vibrava
        * Evolves at ~~45~~ -> 35
    * Numel
        * Evolves at ~~33~~ -> 25
    * Spheal
        * Evolves at ~~32~~ -> 25
    * Sealeo
        * Evolves at ~~44~~ -> 32
        * Learns rest at ~~39~~ -> 35
    * Cacnea
        * Evolves at ~~32~~ -> 25
    * Snorunt
        * Evolves at ~~42~~ -> 28
    * Azurill
        * Evolves at ~~high friendship~~ -> 14
        * Learns slam at ~~15~~ -> 14
    * Spoink
        * Evolves at ~~32~~ -> 28
    * Meditite
        * Evolves at ~~37~~ -> 25
        * Learns calm mind at ~~28~~ -> 25
    * Swablu
        * Evolves at ~~35~~ -> 25
    * Duskull
        * Evolves at ~~37~~ -> 25
        * Learns pursuit at ~~27~~ -> 25
    * Vigoroth
        * Evolves at ~~36~~ -> 32
    * Gulpin
        * Evolves at ~~26~~ -> 23
    * Loudred
        * Evolves at ~~40~~ -> 30
    * Clamperl
        * Evolves into Huntail with ~~deep sea tooth trade~~ -> 20
        * Evolves into Gorebyss with ~~deep sea scale trade~~ -> water stone
    * Shuppet
        * Evolves at ~~37~~ -> 30
    * Aron
        * Evolves at ~~32~~ -> 25
    * Lairon
        * Evolves at ~~42~~ -> 35
        * Learns protect at ~~37~~ -> 35
    * Lileep
        * Evolves at ~~40~~ -> 35
        * Learns amnesia at ~~36~~ -> 35
    * Anorith
        * Evolves at ~~40~~ -> 35
        * Learns ancient power at ~~37~~ -> 35
    * Bagon
        * Evolves at ~~30~~ -> 26
    * Shelgon
        * Evolves at ~~50~~ -> 35
        * Learns dragon breath at ~~38~~ -> 35
    * Metang
        * Evolves at ~~45~~ -> 35
        * Learns psychic at ~~38~~ -> 35
