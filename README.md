# beech-tree
## Install Conan

```bash
python -m venv env
source ./env/bin/activate
pip install conan
conan profile detect
```

## Build

```bash
# 1) Fetch dependencies
conan install . -s build_type=Release --build=missing

# 2) Generate CMake
cmake --preset conan-release

# 3) build
cmake --build --preset conan-release
```
