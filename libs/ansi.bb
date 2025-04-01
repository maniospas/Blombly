!namespace colors {
    !var black;
    !var red;
    !var green;
    !var yellow;
    !var blue;
    !var purple;
    !var cyan;
    !var white;
    !var lightblack;
    !var lightred;
    !var lightgreen;
    !var lightyellow;
    !var lightblue;
    !var lightpurple;
    !var lightcyan;
    !var lightwhite;
    !var reset;
}

!with colors:
ansi = new {
    final black = "\e[0;30m";
    final red = "\e[0;31m";
    final green = "\e[0;32m";
    final yellow = "\e[0;33m";
    final blue = "\e[0;34m";
    final purple = "\e[0;35m";
    final cyan = "\e[0;36m";
    final white = "\e[0;37m";
    final lightblack = "\e[0;90m";
    final lightred = "\e[0;91m";
    final lightgreen = "\e[0;92m";
    final lightyellow = "\e[0;93m";
    final lightblue = "\e[0;94m";
    final lightpurple = "\e[0;95m";
    final lightcyan = "\e[0;96m";
    final lightwhite = "\e[0;97m";
    final reset = "\e[0m";
}
