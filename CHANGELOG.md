# 📜 Registro de Cambios (CHANGELOG)

Todas las modificaciones notables de este proyecto se documentan en este archivo siguiendo los principios de [Versionado Semántico](https://semver.org/lang/es/).

---

## [1.1.4] - 2026-07-22

### 🚀 Corregido (Fixes Críticos de Compatibilidad Binaria)
- **Compatibilidad 100% Bit a Bit con Azahar PC**: Solucionado el problema por el cual los archivos `.zcci` generados en Switch provocaban el cierre del núcleo Azahar (Tico).
- **Cabecera de Metadatos Z3DS**: Incorporado el bloque obligatorio de 128 bytes `Z3DSMetadata` (`header.metadata_size = 128`), corrigiendo la búsqueda de offsets de tramas.
- **Hashes de Verificación XXH32 por Trama**: Activada la bandera de suma de comprobación por cuadro (`checksumFlag = 1`) en el compresor streaming ZSTD.
- **Firma Mágica del Seek Table (`0x8F92EAB1`)**: Firmado el footer del Seek Table con el identificador `0x8F92EAB1` correspondiente al modo checksum.

---

## [1.1.1] - 2026-07-22

### ⚡ Añadido
- **Detección de Cargador USB-C / Dock**: Integración con el servicio `psmGetChargerType` de libnx para dibujar un icono de rayo dorado (`⚡`) al lado del porcentaje de batería cuando la consola se está cargando.
- **Modal de Confirmación de Sobrescritura**: Advertencia de seguridad con opciones `[X / Y] Sobrescribir` y `[A / B] Cancelar` al intentar comprimir un juego que ya posee un archivo `.zcci` generado en la SD.

### 🎨 Cambios visuales
- Limpieza de etiquetas de texto redundantemente largas en los botones.

---

## [1.1.0] - 2026-07-22

### 👆 Añadido
- **Soporte Táctil Capacitivo Nativo (Touchscreen)**: Implementación completa con el driver capacitivo nativo de libnx (`HidTouchScreenState`).
- Interacción táctil en pestañas principales (`ROMs 3DS` y `Acerca de`), selección de tarjetas de juegos, doble toque para iniciar compresión y selección de botones en ventanas modales.

---

## [1.0.9] - 2026-07-22

### 🎨 Añadido
- **Sistema de Ventanas Emergentes con Cristal Oscuro (*Glassmorphism*)**: Superposición de modales de progreso y finalización sobre una capa semitransparente (`rgba(0, 0, 0, 160)`), manteniendo la barra de estado superior y créditos del footer visibles durante todo el proceso.
- **Salto de Línea en Rutas Largas**: Separación limpia entre la ruta de destino (`sdmc:/tico/roms/3ds/`) y el nombre del archivo generado (`Juego.zcci`).

---

## [1.0.7] - 2026-07-22

### 🌐 Añadido
- **Soporte Tipográfico en Español**: Glifos de mapa de bits personalizados para vocales acentuadas (`á`, `é`, `í`, `ó`, `ú`, `ñ`, `¿`, icono de corazón en rojo `\x03`).
- **Pie de Página Dedicado**: Incorporación de la bandera chilena 🇨🇱 y varita mágica 🪄 en los créditos inferiores.

---

## [1.0.5] - 2026-07-22

### 📊 Añadido
- **Barra Superior de Monitoreo de Hardware**: Reloj del sistema en tiempo real, sensor de temperatura interna/externa (`tsGetTemperatureMilliC`) y porcentaje de carga de la batería.

---

## [1.0.1] - 2026-07-22

### 🖼️ Añadido
- **Interfaz Gráfica Nativa Horizon (Borealis)**: Exploración automática de carpetas SD (`sdmc:/tico/roms/3ds`, `sdmc:/roms/3ds`, `sdmc:/3ds`), renderizado con aceleración SDL2 GPU y barra de progreso de compresión con indicador de velocidad (MB/s) y tiempo restante estimado (ETA).

---

## [1.0.0] - 2026-07-22

### 🎉 Inicial
- Primera versión funcional del motor de compresión de ROMs `.3ds` $\rightarrow$ `.zcci` para Nintendo Switch en entorno devkitA64 + libnx + ZSTD.
