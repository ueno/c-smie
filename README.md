c-smie
======
c-smie is an attempt to port
[SMIE](http://www.gnu.org/software/emacs/manual/html_node/elisp/SMIE.html#SMIE),
an indentation engine in Emacs, aiming for the use in other editing
programs.

Testing
------
```
$ gnome-autogen.sh CFLAGS="-O0 -g3 -Wall"
$ make
$ make check
```

To run an interactive demo (requires GtkSourceView):
```
$ ./editor --grammar tests/test.grammar tests/test.input
```
