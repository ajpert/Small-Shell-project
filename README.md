# Small-Shell-project
a useable shell written in c at OSU<br>
to run: gcc --std=gnu99 -o smallsh main.c token-parser.h token-parser.o <br>
In this program, we take inputs from the user such as: <br>
  <ul>
    <li>cd [dir]</li>
    <li>command* &</li>
    <li>command > some_file</li>
  </ul>
In this shell, commands that are taken care of manulay are:
<ul>
    <li>cd (normal functions)</li>
    <li>exit (clean and exit)</li>
    <li>status</li>
    <li>$$ (showing process id)</li>
    <li>exit</li>
    <li># (comment lines)</li>
    <li> < & > (file direction)</li>
    <li> & (background process)</li>
    <li>CNTR-C (stops all foreground children)</li>
    <li>CNTRL-Z (foreground only mode)</li> 
  </ul>
This program mainly functions on forking off children and executing other statemets<br>
using exec(). Child processes are kept track of during useage and are cleaned at the end.<br>
the token parser file was given to us to use <br>
