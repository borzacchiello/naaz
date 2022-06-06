# naaz

Yet another symbolic execution engine. Based on Ghidra's P-Code. Built for fun.

### Compile (Linux/OSX)

Pull the repo and the submodules:

```
git clone https://github.com/borzacchiello/naaz.git
cd naaz
git submodule update --init
```

Compile third_party libraries:

```
cd third_party
./build.sh
```

Compile naaz:

```
cd ..
mkdir build
cd build
cmake ..
make -j`nproc`
```

The command line tools are under the directory `build/tools/`

### How to Use

naaz comes with two command line utilities:
- `naaz_finder` which looks for a state that reaches a given address.
- `naaz_path_generator`, which generates inputs that covers multiple paths.

examples on how to use this tools can be found in [this](https://github.com/borzacchiello/naaz-tests) repo.

### ToDo

- [ ] Implement COW memory
- [ ] Implement handlers for all P-Code statements
- [ ] Syscalls support
- [ ] Windows/PE support
- [ ] More library models
- [ ] Support more loaders (e.g., one based on rizin)
- [ ] Support for library loading
- [ ] Bindings for Python
