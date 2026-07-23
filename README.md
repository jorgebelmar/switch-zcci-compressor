# 🎮 3DS to ZCCI Converter for Nintendo Switch

[![Build Status](https://github.com/jorgebelmar/switch-zcci-compressor/actions/workflows/build.yml/badge.svg)](https://github.com/jorgebelmar/switch-zcci-compressor/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Release](https://img.shields.io/badge/version-v1.1.4-brightgreen.svg)](CHANGELOG.md)

*Read this in: [Español](#-español) | [English](#-english)*

---

## 🇪🇸 Español

Aplicación Homebrew nativa para Switch que permite comprimir ROMs de 3DS Desencriptadas (`.3ds` / `.cci`) al formato optimizado **`.zcci` (ZSTD Seekable Format)** directamente en la tarjeta SD de la consola. 

Los archivos `.zcci` generados son **100% compatibles bit a bit con la especificación oficial de Azahar / Citra**, permitiendo reducir el tamaño de las ROMs hasta en un 50% para su emulación directa en el núcleo **Azahar (Tico)** sin descompresión previa ni retardos.

### 🌟 Características Principales

- **⚡ Compresión ZSTD Seekable Nativa**: Compresión por bloques independientes de 256 KB con hashes `XXH32` por trama (`checksumFlag = 1`), bloque de metadatos `Z3DSMetadata` (128 bytes) y tabla de búsqueda Seek Table (`0x8F92EAB1`).
- **📱 Interfaz Táctil Capacitiva (Touchscreen)**: Control 100% táctil nativo mediante `libnx` + soporte completo para Joy-Cons y Pro Controller.
- **🔋 Monitoreo de Hardware en Tiempo Real**: Reloj del sistema, sensor de temperatura (`tsGetTemperatureMilliC`) y porcentaje de batería con indicador de carga USB-C / Dock (`⚡`).
- **🛡️ Ventana de Protección contra Sobrescritura**: Advertencia de seguridad antes de sobrescribir archivos `.zcci` existentes.
- **✨ Diseño Horizon Dark Glass**: Interfaz estilo *glassmorphism* emergente con barra de estado y créditos permanentes.

### 📂 Instalación en la Switch

1. Descarga **`3ds_to_zcci_compressor.nro`** desde la sección [Releases](../../releases).
2. Copia el archivo `.nro` en la carpeta `switch/` de tu tarjeta SD:
   ```text
   sdmc:/switch/3ds_to_zcci_compressor.nro
   ```
3. Ejecuta la aplicación desde el **Homebrew Launcher (nx-hbloader)**.
4. Coloca tus ROMs `.3ds` en cualquier carpeta detectada automáticamente:
   - `sdmc:/tico/roms/3ds/`
   - `sdmc:/roms/3ds/`
   - `sdmc:/3ds/`

---

## 🇬🇧 English

Native Homebrew application for Switch that compresses decrypted 3DS ROMs (`.3ds` / `.cci`) into the optimized **`.zcci` (ZSTD Seekable Format)** directly on the console's SD card.

The generated `.zcci` files are **100% bit-exact compatible with the official Azahar / Citra specification**, reducing ROM sizes from 1 GB down to ~300 MB for instant, direct emulation in the **Azahar (Tico)** core without prior extraction or lag.

### 🌟 Key Features

- **⚡ Native ZSTD Seekable Compression**: Independent 256 KB block streaming compression with per-frame `XXH32` checksums (`checksumFlag = 1`), 128-byte `Z3DSMetadata` header block, and Seek Table index (`0x8F92EAB1`).
- **📱 Native Capacitive Touchscreen**: 100% touch control via `libnx` + full support for Joy-Cons and Pro Controller.
- **🔋 Real-Time Hardware Monitoring**: System clock, hardware temperature sensor (`tsGetTemperatureMilliC`), and battery level with USB-C / Dock charging indicator (`⚡`).
- **🛡️ Overwrite Protection Modal**: Safety warning prompt before overwriting existing `.zcci` files.
- **✨ Horizon Dark Glass Design**: Pop-up *glassmorphism* interface with persistent top status bar and footer credits.

### 📂 Installation on Switch

1. Download **`3ds_to_zcci_compressor.nro`** from the [Releases](../../releases) tab.
2. Copy the `.nro` file to the `switch/` directory on your SD card:
   ```text
   sdmc:/switch/3ds_to_zcci_compressor.nro
   ```
3. Launch the application from **Homebrew Launcher (nx-hbloader)**.
4. Place your `.3ds` ROMs in any automatically detected folder:
   - `sdmc:/tico/roms/3ds/`
   - `sdmc:/roms/3ds/`
   - `sdmc:/3ds/`

---

## 🛠️ Compilación / Building

```bash
git clone https://github.com/jorgebelmar/switch-zcci-compressor.git
cd switch-zcci-compressor

make clean
make
```

---

## 📜 Licencia / License

Distributed under the **MIT License**. See [LICENSE](LICENSE) for details.

- **Developer / Desarrollado por**: [elcoke.cl](https://elcoke.cl)
- Special thanks to the Switch homebrew community, Azahar & Citra Project creators, devkitPro / libnx team, and Borealis framework developers.

> *"Desde CholChol - Chile 🇨🇱 con amor para los amantes de la magia 🪄"*
