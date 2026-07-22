# 🎮 3DS to ZCCI Converter for Nintendo Switch

[![Build Status](https://github.com/elcoke-cl/switch-zcci-compressor/actions/workflows/build.yml/badge.svg)](https://github.com/elcoke-cl/switch-zcci-compressor/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Release](https://img.shields.io/badge/version-v1.1.4-brightgreen.svg)](CHANGELOG.md)

Aplicación Homebrew nativa para Nintendo Switch que permite comprimir ROMs de Nintendo 3DS (`.3ds` / `.cci`) al formato optimizado **`.zcci` (ZSTD Seekable Format)** directamente en la tarjeta SD de la consola. 

Los archivos `.zcci` generados son **100% compatibles bit a bit con la especificación oficial de Azahar / Citra**, permitiendo reducir el tamaño de las ROMs de 1 GB a ~300 MB para su emulación directa en el núcleo **Azahar (Tico)** sin descompresión previa ni retardos.

---

## 🌟 Características Principales

- **⚡ Compresión ZSTD Seekable Nativa**: Compresión multihilo por bloques independientes de 256 KB con firmas de checksum `XXH32` por trama y tabla de búsqueda Seek Table (`0x8F92EAB1`).
- **📱 Interfaz Táctil Capacitiva (Touchscreen)**: Control 100% táctil nativo mediante `libnx` + soporte para Joy-Cons y Pro Controller.
- **🔋 Monitoreo de Hardware en Tiempo Real**: Reloj del sistema, sensor de temperatura (`tsGetTemperatureMilliC`) y porcentaje de batería con indicador de carga USB-C / Dock (`⚡`).
- **🛡️ Ventana de Protección contra Sobrescritura**: Advertencia de seguridad antes de sobrescribir archivos `.zcci` existentes.
- **✨ Diseño Horizon Dark Glass**: Interfaz estilo *glassmorphism* emergente con barra de estado y créditos permanentes.

---

## 📂 Instalación en Nintendo Switch

1. Descarga el ejecutable **`3ds_to_zcci_compressor.nro`** desde la sección [Releases](../../releases).
2. Copia el archivo `.nro` en la carpeta `switch/` de tu tarjeta SD:
   ```text
   sdmc:/switch/3ds_to_zcci_compressor.nro
   ```
3. Ejecuta la aplicación desde el **Homebrew Launcher (nx-hbloader)** en tu Nintendo Switch.
4. Coloca tus ROMs `.3ds` en cualquiera de las siguientes carpetas detectadas automáticamente:
   - `sdmc:/tico/roms/3ds/`
   - `sdmc:/roms/3ds/`
   - `sdmc:/3ds/`

---

## 🛠️ Compilación desde el Código Fuente

### Requisitos Prácticos
- Toolchain de **devkitPro** (`devkitA64`, `libnx`, `switch-tools`, `switch-sdl2`).
- MSYS2 (Windows) o Docker (Linux/macOS).

### Pasos de Compilación

```bash
git clone https://github.com/elcoke-cl/switch-zcci-compressor.git
cd switch-zcci-compressor

make clean
make
```

El resultado de la compilación generará el binario `3ds_to_zcci_compressor.nro` listo para transferir.

---

## 📜 Licencia y Agradecimientos

Distribuido bajo la Licencia **MIT**. Consulta el archivo [LICENSE](LICENSE) para más detalles.

- **Desarrollado por**: [elcoke.cl](https://elcoke.cl)
- Agradecimientos especiales a la comunidad homebrew de Nintendo Switch, creadores del emulador Azahar & Citra Project, devkitPro / libnx y el equipo detrás del framework Borealis.

> *"Desde CholChol - Chile 🇨🇱 con amor para los amantes de la magia 🪄"*
