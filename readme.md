#A commandline dotfiles manager


###dotfiles --git REPO [BLOCK]
   if BLOCK is not defined, clone REPO into
   ~/.dotfiles-remote, overwriting if necessary
   otherwise, it will do a similar clone, but
   the provided BLOCK and corresponding files
   will be copied into ~/.dotfiles, ONLY if that
   block does not already exist in ~/.dotfiles/dotfiles
   if ~/.dotfiles does not exist, 'dotfiles --init'
   will be run before

###dotfiles --sync
   refresh the symlinks in the home directory

###dotfiles --init
   if no DIR is provided, create ~/.dotfiles, otherwise
   copies the DIR to ~/.dotfiles and creates a
   symlink to ~/.dotfiles in the old location.
   then finds a dotfiles file in ~/.dotfiles/
   and initializes from it. 

###dotfiles --track FILE NAME [--block BLOCK] [--host]
   moves ~/FILE to ~/.dotfiles/NAME and creates a
   symlink named ~/FILE pointing to ~/.dotfiles/NAME
   if the --block flag and name are specified,
   a symlink will be tracked in that block
   otherwise, it will create a new block named
   BLOCK_N, where N is some number, and add it there
   if the host flag is specified, then the current
   user@host sub-block will be where the sumlink is
   tracked. NOTE: this will NOT track symlinks!

###dotfiles --list [BLOCK] [--host USER@HOST]
   lists all blocks by name, or if a BLOCK name is
   specified, list only the contents of that block
   if the host flag and HOST are provided, it will
   list all the files for that user@host instead of
   the current user#host


