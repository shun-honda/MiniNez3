# MiniNez3

## Requirement
Need to install cmake
### OSX
```
brew install cmake
```
### Ubuntu
```
sudo apt-get install cmake
```

## Build
You'll have to make a few steps. Type in:
```
  $ mkdir build
  $ cd build
  $ cmake ..
  $ make
  $ execute ./build/mininez
```

## Execution
You can execute sample as follows:
```
  $ ./build/mininez -g sample/bytecode/math.bin -i sample/input/math.txt -t tree
```