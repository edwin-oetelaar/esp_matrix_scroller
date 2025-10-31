#include <std/string>

struct color {
    u8 r;
    u8 g;
    u8 b;
} [[hex::inline_visualize("color", r, g, b, 0xff)]];

struct config {
    u32 text_length;
    char text[text_length];
    color fcolor;
    color bcolor;
    u16 speed;
};

config myconfig @ 0x00;