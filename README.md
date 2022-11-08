# Full Moon

## TODO

- [ ] Editor
- - [ ] Wave
- - [ ] Song
- - [ ] Launch from editor, at the current map if one is open
- - [ ] New datalists (NewMapCommandModal)
- - - [ ] Flags (for map "cellif" command)
- - - [ ] Sprite controller
- - - [ ] Events
- - - [ ] Hooks
- [ ] Targets
- - [ ] Web
- - [ ] Linux DRM
- - [ ] Windows
- - [x] MacOS
- - [ ] Thumby
- - [ ] Picosystem
- - [ ] Mac 68k
- - [ ] Pippin
- - [ ] Playdate
- - [ ] PocketStar
- [ ] HP, damage
- [ ] Save game
- [ ] Spriteless treadle and stompbox
- [ ] Map-based events
- [ ] Weapons, first pass.
- - [x] Broom
- - [ ] Umbrella
- - [x] Wand
- - [x] Violin
- - [x] Chalk
- - [x] Pitcher
- - [x] Feather
- - [ ] Matches
- - [x] Bell
- - [ ] Corn
- - [ ] Shovel
- - [ ] Compass
- - [ ] Coins
- [ ] Spells
- [ ] Spell repudiation
- [ ] Change hat
- [ ] Sound effects
- [ ] Clean up music
- [ ] Sprites
- - [ ] Slicer
- - [ ] Bonfire
- - [ ] Firewall
- - [ ] Raccoon
- - [ ] Sea monster
- - [ ] Fire nozzle
- - [ ] Soulballs

## Defects

- [x] fmn_game_cast_song() is getting re-called on map changes, with the most recent song.
- [x] Sometimes a buzz when changing screens. Related to the cast_song thing? ...no. Sounds like the metronome.
- [ ] Playing violin, prevent new notes from appearing at the left, sometimes they do briefly.
- [ ] Ride broom and turn during neighbor screen transition, image gets doubled.
