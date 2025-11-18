![https://github.com/bakkeby/dack/wiki/images/dack.jpg](https://github.com/bakkeby/dack/wiki/images/dack.jpg)

dack is an X11 wallpaper setter that can apply filters and other effects to images during runtime.

The program is controlled using a configuration file rather than command line arguments, and the
main feature is being able to apply a multitude of filters on wallpapers during runtime to create
various effects.

As a simple example the desktop background can be made black and white while the source image
remains unchanged.

dack can be customized via runtime configuration `dack.cfg` placed in `~/.config/dack/` or
`XDG_CONFIG_HOME` equivalent.

Optionally a configuration file can be specified using the `DACK_CONFIG_PATH` environment variable.

A configuration file passed as a command line argument will take precedence.

More details can be found on the [wiki](https://github.com/bakkeby/dack/wiki)
