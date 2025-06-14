KASM syntax vim plugin
-----------------------
* Download the kasm.vim file
* Move it to ~/.vim/syntax/ (Create syntax if it does not exist)
* Add the following line to your .vimrc to automatically load the plugin for .kasm files:
	- filetype plugin on
	- syn on
	- au BufRead,BufNewFile *.kasm set filetype=kasm