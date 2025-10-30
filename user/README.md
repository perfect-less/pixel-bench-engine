## Setup

### 1. Create a new directory
```bash
mkdir my_game
cd my_game
```

### 2. Create a `subprojects` directory and put pixbench's wrap file there
```bash
mkdir subprojects && \
cd subprojects && \
wget https://raw.githubusercontent.com/perfect-less/pixel-bench-engine/refs/heads/main/user/pixel-bench-engine.wrap && \
cd ..
```

### 3. Get `meson.build` file
```bash
wget https://raw.githubusercontent.com/perfect-less/pixel-bench-engine/refs/heads/main/user/meson.build
```

### 4. Get the sample `init.cpp` file
```bash
mkdir src && cd src && \
wget https://raw.githubusercontent.com/perfect-less/pixel-bench-engine/refs/heads/main/user/src/init.cpp && \
cd ..
```

### 5. Setup `build` directory
```bash
meson setup build
```

### 6. Compile the game
```bash
meson compile -C build
```

### 7. Run the game
```bash
./build/game
```
