# Qrview

A simple applications to view and generate QR codes!
![alt text](./assets/qr.png "QR")

### Quick Start
```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release .. # It may take a little bit!
make -j 4

# running the application
./qrview
```

### Controls
| Key          | Description                |
| ------------ | -------------------------- |
| ESC          | Toggle editor window       |
| LCtrl+S      | Save QR code               |


### Building for Escripten with Linux
```bash
# clone the toolchain
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

cd ..

mkdir build
cd build

# building the project
emcmake cmake -DCMAKE_BUILD_TYPE=Release ..
emmake make

# running the application
emrun ./qrview.html
```

#### Resources
- https://github.com/zxing/zxing/wiki/Barcode-Contents
- https://github.com/nayuki/QR-Code-generator
