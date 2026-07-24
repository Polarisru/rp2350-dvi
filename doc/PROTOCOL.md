# UART Protocol

Serial port: **250000 8N1**, pins GPIO 0 (TX) / GPIO 1 (RX).

Each command is a single line terminated by `\n` (bare LF or CRLF both accepted).  
Maximum line length: **127 characters** (excluding the newline).  
All numeric fields are **unsigned decimal integers** — no leading zeros, no spaces, no sign.

---

## Command reference

### `0` — Frame header

```text
0:<num_lines>:<font>
```

Announces the start of a new text frame.

| Field | Type | Range | Description |
|---|---|---:|---|
| `num_lines` | uint8 | 0..255 | Number of text lines that will follow |
| `font` | uint8 | 0..2 | Font size; see [Font table](#font-table) |

Must be sent before any numbered text lines. Resets the receiver's line counter.

**Example**

```text
0:4:2
```

---

### `<N>` — Numbered text line

```text
<N>:<text>:<color>
```

Draws a line of text at a logical row index. Position on screen is determined by the application using the font from the preceding header.

| Field | Type | Range | Description |
|---|---|---:|---|
| `N` | uint16 | 1..65535 | Line index |
| `text` | string | — | Arbitrary text; may contain colons |
| `color` | uint8 | 0..7 | VGA color index; see [Color table](#color-table) |

The parser splits on the **last** colon to locate `color`, so `text` may contain colons freely.

**Examples**

```text
1:Hello, world!:7
2:Status: OK:3
```

---

## Legacy and 8-bit commands

Uppercase drawing commands use the legacy VGA palette range `0..7` and return the original message types.

Lowercase drawing commands use identical payload layouts, accept colors in the range `0..255`, and return distinct 8-bit message types.

| Command | Color range | Message type |
|---|---:|---|
| `F:<color>` | 0..7 | `UART_RX_CMD_FILL` |
| `f:<color>` | 0..255 | `UART_RX_CMD_FILL_8BIT` |
| `R:<x1>:<y1>:<x2>:<y2>:<color>` | 0..7 | `UART_RX_CMD_RECT` |
| `r:<x1>:<y1>:<x2>:<y2>:<color>` | 0..255 | `UART_RX_CMD_RECT_8BIT` |
| `B:<x1>:<y1>:<x2>:<y2>:<width>:<color>` | 0..7 | `UART_RX_CMD_BOX` |
| `b:<x1>:<y1>:<x2>:<y2>:<width>:<color>` | 0..255 | `UART_RX_CMD_BOX_8BIT` |
| `T:<x>:<y>:<color>:<font>:<text>` | 0..7 | `UART_RX_CMD_TEXT` |
| `t:<x>:<y>:<color>:<font>:<text>` | 0..255 | `UART_RX_CMD_TEXT_8BIT` |

The application uses `message->type` to distinguish legacy palette commands from direct 8-bit color commands.

---

### `F` — Fill screen, legacy color

```text
F:<color>
```

Fills the entire framebuffer with a legacy VGA palette color.

| Field | Type | Range | Description |
|---|---|---:|---|
| `color` | uint8 | 0..7 | VGA color index; see [Color table](#color-table) |

**Example**

```text
F:0
```

---

### `f` — Fill screen, 8-bit color

```text
f:<color>
```

Fills the entire framebuffer with a direct 8-bit color. The receiver returns `UART_RX_CMD_FILL_8BIT`.

| Field | Type | Range | Description |
|---|---|---:|---|
| `color` | uint8 | 0..255 | Direct color value interpreted by the application |

**Example**

```text
f:128
```

---

### `R` — Draw rectangle, legacy color

```text
R:<x1>:<y1>:<x2>:<y2>:<color>
```

Draws a filled rectangle using a legacy VGA palette color. Corner order does not matter; coordinates are sorted internally. Out-of-bounds coordinates are clamped to the screen edges.

| Field | Type | Range | Description |
|---|---|---:|---|
| `x1` | uint16 | 0..719 | X coordinate of the first corner |
| `y1` | uint16 | 0..479 | Y coordinate of the first corner |
| `x2` | uint16 | 0..719 | X coordinate of the opposite corner |
| `y2` | uint16 | 0..479 | Y coordinate of the opposite corner |
| `color` | uint8 | 0..7 | VGA color index |

**Example**

```text
R:10:20:200:100:3
```

---

### `r` — Draw rectangle, 8-bit color

```text
r:<x1>:<y1>:<x2>:<y2>:<color>
```

Draws a filled rectangle with a direct 8-bit color. The receiver returns `UART_RX_CMD_RECT_8BIT`.

| Field | Type | Range | Description |
|---|---|---:|---|
| `x1` | uint16 | 0..719 | X coordinate of the first corner |
| `y1` | uint16 | 0..479 | Y coordinate of the first corner |
| `x2` | uint16 | 0..719 | X coordinate of the opposite corner |
| `y2` | uint16 | 0..479 | Y coordinate of the opposite corner |
| `color` | uint8 | 0..255 | Direct color value interpreted by the application |

**Example**

```text
r:10:20:200:100:196
```

---

### `B` — Draw box, legacy color

```text
B:<x1>:<y1>:<x2>:<y2>:<width>:<color>
```

Draws a rectangular frame using a legacy VGA palette color.

| Field | Type | Range | Description |
|---|---|---:|---|
| `x1` | uint16 | 0..719 | X coordinate of the first corner |
| `y1` | uint16 | 0..479 | Y coordinate of the first corner |
| `x2` | uint16 | 0..719 | X coordinate of the opposite corner |
| `y2` | uint16 | 0..479 | Y coordinate of the opposite corner |
| `width` | uint8 | 0..255 | Line width in pixels |
| `color` | uint8 | 0..7 | VGA color index |

**Example**

```text
B:10:20:200:100:2:3
```

---

### `b` — Draw box, 8-bit color

```text
b:<x1>:<y1>:<x2>:<y2>:<width>:<color>
```

Draws a rectangular frame using a direct 8-bit color. The receiver returns `UART_RX_CMD_BOX_8BIT`.

| Field | Type | Range | Description |
|---|---|---:|---|
| `x1` | uint16 | 0..719 | X coordinate of the first corner |
| `y1` | uint16 | 0..479 | Y coordinate of the first corner |
| `x2` | uint16 | 0..719 | X coordinate of the opposite corner |
| `y2` | uint16 | 0..479 | Y coordinate of the opposite corner |
| `width` | uint8 | 0..255 | Line width in pixels |
| `color` | uint8 | 0..255 | Direct color value interpreted by the application |

**Example**

```text
b:10:20:200:100:2:45
```

---

### `T` — Draw text, legacy color

```text
T:<x>:<y>:<color>:<font>:<text>
```

Renders UTF-8 text at a pixel-precise position using a bitmap font and a legacy VGA palette color.

| Field | Type | Range | Description |
|---|---|---:|---|
| `x` | uint16 | 0..719 | Pixel X coordinate of the first glyph |
| `y` | uint16 | 0..479 | Pixel Y coordinate of the first glyph |
| `color` | uint8 | 0..7 | VGA color index |
| `font` | uint8 | 0..2 | Font size; see [Font table](#font-table) |
| `text` | string | — | Text to render; may contain colons |

**Example**

```text
T:50:120:7:1:Temperature: 23.5 C
```

---

### `t` — Draw text, 8-bit color

```text
t:<x>:<y>:<color>:<font>:<text>
```

Renders UTF-8 text at a pixel-precise position using a direct 8-bit color. The receiver returns `UART_RX_CMD_TEXT_8BIT`.

| Field | Type | Range | Description |
|---|---|---:|---|
| `x` | uint16 | 0..719 | Pixel X coordinate of the first glyph |
| `y` | uint16 | 0..479 | Pixel Y coordinate of the first glyph |
| `color` | uint8 | 0..255 | Direct color value interpreted by the application |
| `font` | uint8 | 0..2 | Font size; see [Font table](#font-table) |
| `text` | string | — | Text to render; may contain colons |

**Example**

```text
t:50:120:255:1:Temperature: 23.5 C
```

---

## Color table

The legacy uppercase commands use this 3-bit VGA palette.

| Index | Name |
|---:|---|
| 0 | Black |
| 1 | Red |
| 2 | Green |
| 3 | Yellow |
| 4 | Blue |
| 5 | Magenta |
| 6 | Cyan |
| 7 | White |

Colors are 3-bit RGB, one bit per channel.

---

## Font table

| Index | Glyph height |
|---:|---|
| 0 | 64 px |
| 1 | 48 px |
| 2 | 32 px |

---

## Error handling

If a line cannot be parsed because it has a missing field, non-numeric field, out-of-range value, or buffer overflow, the receiver calls the registered error callback with the offending raw token and a short reason string. The line is discarded and reception continues normally from the next newline.
