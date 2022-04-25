# c-atom
see-atom, carbon-atom, C language-atom is a scalable framework to create embedded 
systems faster by the combination of the top level declarative programming of the behaviour;
model-based enabled design; service code generation.

- written on C language;
- uses POSIX for system calls;
- designed to be hardware and software platforms agnostic.

# Demos

1. [Autopiloting of the flight simulator's model of Cessna 172.](https://github.com/ctlst-tech/c-atom)
2. Popular open source hardware adaptation and quad-rotor drone example (coming soon).

# How it works

Library has following major blocks:
- **flow** - block to arrange computational graphs as a sequence of atomic reusable (C lang coded) functions.
- **fsm** - finite state machine block, operates by states, transitions and actions on states and transitions.
- **ibr** - interface bridge - designed to take care of converting information from and to other devices.
- **swsys** - software system description layer; allocates functions and other blocks into tasks and process.

The foundation of the **c-atom** is [Embedded Software Bus (ESWB)](https://github.com/ctlst-tech/eswb) library. 
ESWB creates uniform way of functions to communicate between each other: inside thread, between threads, between processes.
Stands as the only form of inter process communication inside **c-atom** controlled domain. 


# Installation

There are following dependencies and necessary tooling:
1. bison and flex as code for parser generation of **fsm**. 
2. Catch2 as a testing framework

## Install Catch2
```shell
git clone https://github.com/catchorg/Catch2.git
cd Catch2
git checkout v2.13.9
cmake -Bbuild -H. -DBUILD_TESTING=OFF
sudo cmake --build build/ --target install
```

## Install bison & flex

### Ubuntu

```sudo apt update && apt install bison flex```

### MacOS

```shell
brew install bison
```

Make sure you have new bison in ```/usr/local/bin/bison```.
C-atom's CMakeLists.txt configuration has to override existing bison in a system.
Either adjust ```fsminst/fsmlib/CMakeLists.txt```.
Or create a symbolic link.
```sudo ln -s /opt/homebrew/opt/bison/bin/bison /usr/local/bin/bison```

## Building and running

Using of IDE to configure, build and run **c-atom** runner is recommended (CLion, VScode).


