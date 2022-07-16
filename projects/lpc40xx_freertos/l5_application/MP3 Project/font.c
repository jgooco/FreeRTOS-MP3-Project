#include "font.h"
#include "uart_lab.h"

// UPPER CASE LETTERS
void char_A() { uart_write(0x41); }
void char_B() { uart_write(0x42); }
void char_C() { uart_write(0x43); }
void char_D() { uart_write(0x44); }
void char_E() { uart_write(0x45); }
void char_F() { uart_write(0x46); }
void char_G() { uart_write(0x47); }
void char_H() { uart_write(0x48); }
void char_I() { uart_write(0x49); }
void char_J() { uart_write(0x4A); }
void char_K() { uart_write(0x4B); }
void char_L() { uart_write(0x4C); }
void char_M() { uart_write(0x4D); }
void char_N() { uart_write(0x4E); }
void char_O() { uart_write(0x4F); }
void char_P() { uart_write(0x50); }
void char_Q() { uart_write(0x51); }
void char_R() { uart_write(0x52); }
void char_S() { uart_write(0x53); }
void char_T() { uart_write(0x54); }
void char_U() { uart_write(0x55); }
void char_V() { uart_write(0x56); }
void char_W() { uart_write(0x57); }
void char_X() { uart_write(0x58); }
void char_Y() { uart_write(0x59); }
void char_Z() { uart_write(0x5A); }

// LOWER CASE LETTERS
void char_a() { uart_write(0x61); }
void char_b() { uart_write(0x62); }
void char_c() { uart_write(0x63); }
void char_d() { uart_write(0x64); }
void char_e() { uart_write(0x65); }
void char_f() { uart_write(0x66); }
void char_g() { uart_write(0x67); }
void char_h() { uart_write(0x68); }
void char_i() { uart_write(0x69); }
void char_j() { uart_write(0x6A); }
void char_k() { uart_write(0x6B); }
void char_l() { uart_write(0x6C); }
void char_m() { uart_write(0x6D); }
void char_n() { uart_write(0x6E); }
void char_o() { uart_write(0x6F); }
void char_p() { uart_write(0x70); }
void char_q() { uart_write(0x71); }
void char_r() { uart_write(0x72); }
void char_s() { uart_write(0x73); }
void char_t() { uart_write(0x74); }
void char_u() { uart_write(0x75); }
void char_v() { uart_write(0x76); }
void char_w() { uart_write(0x77); }
void char_x() { uart_write(0x78); }
void char_y() { uart_write(0x79); }
void char_z() { uart_write(0x7A); }

// NUMBERS
void char_0() { uart_write(0x30); }
void char_1() { uart_write(0x31); }
void char_2() { uart_write(0x32); }
void char_3() { uart_write(0x33); }
void char_4() { uart_write(0x34); }
void char_5() { uart_write(0x35); }
void char_6() { uart_write(0x36); }
void char_7() { uart_write(0x37); }
void char_8() { uart_write(0x38); }
void char_9() { uart_write(0x39); }

// SPECIAL CHARACTERS
void char_dquote() { uart_write(0x22); }
void char_squote() { uart_write(0x27); }
void char_comma() { uart_write(0x2C); }
void char_qmark() { uart_write(0x3F); }
void char_excl() { uart_write(0x21); }
void char_at() { uart_write(0x40); }
void char_undersc() { uart_write(0x5F); }
void char_star() { uart_write(0x2A); }
void char_hash() { uart_write(0x23); }
void char_percent() { uart_write(0x25); }
void char_amper() { uart_write(0x26); }
void char_parenthL() { uart_write(0x28); }
void char_parenthR() { uart_write(0x29); }
void char_plus() { uart_write(0x2B); }
void char_minus() { uart_write(0x2D); }
void char_div() { uart_write(0x2F); }
void char_colon() { uart_write(0x3A); }
void char_scolon() { uart_write(0x3B); }
void char_less() { uart_write(0x3C); }
void char_greater() { uart_write(0x3E); }
void char_equal() { uart_write(0x3D); }
void char_bracketL() { uart_write(0x5B); }
void char_backslash() { uart_write(0x5C); }
void char_bracketR() { uart_write(0x1D); }
void char_caret() { uart_write(0x5E); }
void char_bquote() { uart_write(0x60); }
void char_braceL() { uart_write(0x7B); }
void char_braceR() { uart_write(0x7D); }
void char_bar() { uart_write(0x7C); }
void char_tilde() { uart_write(0x7E); }
void char_space() { uart_write(0x20); }
void char_period() { uart_write(0x2E); }
void char_dollar() { uart_write(0x24); }
