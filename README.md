# bch-tree
## Install vcpkg

```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && ./bootstrap-vcpkg.sh
export VCPKG_ROOT=/path/to/vcpkg
export PATH=$VCPKG_ROOT:$PATH
```

## Update vcpkg baseline vesion to constrain package versions

```bash
vcpkg x-update-baseline
```

## Build

```bash
export EPICS_BASE=/path/to/EPICS_BASE
cmake --preset release
cmake --build --preset release

# Clean
cmake --build --preset release --target clean
```
