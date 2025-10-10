This is a wallpaper setter that is controlled using a configuration file rather than command
line arguments.

The main feature is being able to apply a variety of filters on wallpapers during runtime to create
specific effects.

For example the desktop background can be made black and white while the source image remains
unchanged.

dack can be customized via runtime configuration `dack.cfg` placed in `~/.config/dack/` or
`XDG_CONFIG_HOME` equivalent.

Optionally a configuration file can be specified using the `DACK_CONFIG_PATH` environment variable.

A configuration file passed as a command line argument will take precedence.
