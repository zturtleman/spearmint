<img src="https://raw.githubusercontent.com/zturtleman/spearmint/master/misc/spearmint_text.png" width="256">

**Spearmint** is a fork of [ioquake3](https://github.com/ioquake/ioq3) with two main goals; 1) provide a flexible engine for creating new games and mods, 2) support features from (and running) various id Tech 3-based games.

Spearmint can be used to play Quake III Arena, Quake III: Team Arena, and Turtle Arena. Progress has been made toward running Return to Castle Wolfenstein (MP) and Wolfenstein: Enemy Territory, there is still quite a bit left before it's possible though. Spearmint is not compatible with existing mods (the QVM/DLL files) or demos (game recordings) for any game.

New Spearmint *0.X* releases will likely break VM and demo compatibility with previous releases.

The source code for the Spearmint Quake 3 game, cgame, and ui code and QVM compiler is at [zturtleman/mint-arena](https://github.com/zturtleman/mint-arena/). Map editor and map compiler are available at https://icculus.org/gtkradiant/.

## Download

Pre-built packages for Windows, GNU/Linux, and Mac OS X are available at the [Spearmint website](http://spearmint.pw).


## Discuss

  * [Magical Clover Forum](https://forum.clover.moe) (supports GitHub login)
  * [#spearmint on chat.freenode.net](irc://chat.freenode.net/#spearmint)


## Documentation

  * [Spearmint wiki](https://github.com/zturtleman/spearmint/wiki)


## Git branches

* `master` branch is compatible with Spearmint 0.4.
* `devil` branch is for development (devil-op-mint) that is not compatible with the current release &mdash; it may be out of date compared to master.
* `coverity_scan` branch is for automatically running [Coverity Scan](https://scan.coverity.com/) on [Travis CI](https://travis-ci.org).
* `gh-pages` branch is the [Spearmint website](http://spearmint.pw).


## License

Spearmint is licensed under a [modified version of the GNU GPLv3](COPYING.txt#L625) (or at your option, any later version). The license is also used by Return to Castle Wolfenstein, Wolfenstein: Enemy Territory, and Doom 3.


## Credits

* Zack Middleton (main developer)
* Tobias Kuehnhammer (feedback / bug reports / Bot AI fixes)
* And other contributors

Spearmint contains code from;
* Quake 3 - id Software
* ioquake3 - ioquake3 contributors
* RTCW SP - Gray Matter Interactive
* RTCW MP - Nerve Software
* Wolfenstein: Enemy Territory - Splash Damage
* Tremulous - Dark Legion Development
* World of Padman - Padworld Entertainment
* [ioquake3 Elite Force MP patch](http://thilo.kickchat.com/efport-progress/) - Thilo Schulz
* NetRadiant's q3map2 - Rudolf Polzer
* OpenArena - OpenArena contributors
* OpenMoHAA - OpenMoHAA contributors
* Xreal (triangle mesh collision) - Robert Beckebans


## Contributing

Please submit all patches at the [Magical Clover Forum](https://forum.clover.moe)
or as a GitHub pull request.

The focus for Spearmint is to develop a stable base suitable for further
development and provide players with the same Quake 3 game play experience
they've had for years.

