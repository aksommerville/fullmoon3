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
- - - [ ] Audio
- - [ ] Linux DRM
- - [ ] Windows
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
- [ ] Spells
- - [ ] Wind
- - [ ] Rain
- - [ ] Slow motion
- - [ ] Opening
- - [ ] Invisibility
- - [ ] (song) Healing
- - [ ] (song) Trailhead teleport
- - [ ] (song) Home teleport
- - [ ] (song) A/B flip
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
- [ ] Songs dropping notes or something.
- [ ] Wasm build exports like every function, dozens or hundreds of them. Find out how to pare that down.
