# Image Assets

This project keeps image decoding outside the firmware. Convert PNG/JPG/BMP files on the PC into row-major 1bpp C headers, then let the display service send that bitmap to the ST7305 driver.

## Install Converter Dependency

```powershell
python -m pip install pillow
```

## Convert A Color Image

The current Waveshare RLCD panel configuration is `300x400`, so generate assets at the same size:

```powershell
python tools/image_to_1bpp.py assets/images/logo.png products/speaker/resources/images/logo_1bpp.h --width 300 --height 400 --dither --name logo_1bpp
```

Without dithering, use a fixed threshold:

```powershell
python tools/image_to_1bpp.py assets/images/logo.png products/speaker/resources/images/logo_1bpp.h --width 300 --height 400 --threshold 150 --name logo_1bpp
```

If the image appears inverted on the reflective LCD, regenerate it with:

```powershell
python tools/image_to_1bpp.py assets/images/logo.png products/speaker/resources/images/logo_1bpp.h --width 300 --height 400 --dither --invert --name logo_1bpp
```

The generated header contains:

```c
#define LOGO_1BPP_WIDTH 300
#define LOGO_1BPP_HEIGHT 400
static const uint8_t logo_1bpp_data[] = { ... };
```

## Draw From Firmware

Include the generated header from a product app, then pass the bitmap through the display service:

```c
#include "display_service.h"
#include "logo_1bpp.h"

ESP_ERROR_CHECK(display_service_show_bitmap_1bpp(
    logo_1bpp_data,
    LOGO_1BPP_WIDTH,
    LOGO_1BPP_HEIGHT,
    false));
```

## Data Format

The generated bitmap is simple row-major 1bpp:

- One bit per pixel.
- Most significant bit first in each byte.
- `1` means bright/on, `0` means dark/off before the optional firmware invert flag.
- Row stride is `(width + 7) / 8`.

Only the ST7305 driver knows how to repack that row-major bitmap into the chip's RAMWR byte layout.
