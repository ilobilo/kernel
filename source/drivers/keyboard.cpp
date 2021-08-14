#include <stdint.h>
#include <system/idt/idt.hpp>
#include <include/string.hpp>
#include <include/io.hpp>
#include <drivers/terminal.hpp>
#include <drivers/keyboard.hpp>

bool isCapsLock = false;
bool isShiftPressed = false;

char get_ascii_char(uint8_t key_code)
{
    if ((isShiftPressed == true && isCapsLock == false) || (isShiftPressed == false && isCapsLock == true))
    {
        switch(key_code)
        {
            case KEY_A : return 'A';
            case KEY_B : return 'B';
            case KEY_C : return 'C';
            case KEY_D : return 'D';
            case KEY_E : return 'E';
            case KEY_F : return 'F';
            case KEY_G : return 'G';
            case KEY_H : return 'H';
            case KEY_I : return 'I';
            case KEY_J : return 'J';
            case KEY_K : return 'K';
            case KEY_L : return 'L';
            case KEY_M : return 'M';
            case KEY_N : return 'N';
            case KEY_O : return 'O';
            case KEY_P : return 'P';
            case KEY_Q : return 'Q';
            case KEY_R : return 'R';
            case KEY_S : return 'S';
            case KEY_T : return 'T';
            case KEY_U : return 'U';
            case KEY_V : return 'V';
            case KEY_W : return 'W';
            case KEY_X : return 'X';
            case KEY_Y : return 'Y';
            case KEY_Z : return 'Z';
            case KEY_1 : return '!';
            case KEY_2 : return '@';
            case KEY_3 : return '#';
            case KEY_4 : return '$';
            case KEY_5 : return '%';
            case KEY_6 : return '^';
            case KEY_7 : return '&';
            case KEY_8 : return '*';
            case KEY_9 : return '(';
            case KEY_0 : return ')';
            case KEY_MINUS : return '_';
            case KEY_EQUAL : return '+';
            case KEY_SQUARE_OPEN_BRACKET : return '{';
            case KEY_SQUARE_CLOSE_BRACKET : return '}';
            case KEY_SEMICOLON : return ':';
            case KEY_BACKSLASH : return '|';
            case KEY_COMMA : return '<';
            case KEY_DOT : return '>';
            case KEY_FORESLHASH : return '?';
            case KEY_SPACE : return ' ';
            case KEY_ENTER : return '\n';
            default : return 0;
        }
    }
    else
    {
        switch(key_code)
        {
            case KEY_A : return 'a';
            case KEY_B : return 'b';
            case KEY_C : return 'c';
            case KEY_D : return 'd';
            case KEY_E : return 'e';
            case KEY_F : return 'f';
            case KEY_G : return 'g';
            case KEY_H : return 'h';
            case KEY_I : return 'i';
            case KEY_J : return 'j';
            case KEY_K : return 'k';
            case KEY_L : return 'l';
            case KEY_M : return 'm';
            case KEY_N : return 'n';
            case KEY_O : return 'o';
            case KEY_P : return 'p';
            case KEY_Q : return 'q';
            case KEY_R : return 'r';
            case KEY_S : return 's';
            case KEY_T : return 't';
            case KEY_U : return 'u';
            case KEY_V : return 'v';
            case KEY_W : return 'w';
            case KEY_X : return 'x';
            case KEY_Y : return 'y';
            case KEY_Z : return 'z';
            case KEY_1 : return '1';
            case KEY_2 : return '2';
            case KEY_3 : return '3';
            case KEY_4 : return '4';
            case KEY_5 : return '5';
            case KEY_6 : return '6';
            case KEY_7 : return '7';
            case KEY_8 : return '8';
            case KEY_9 : return '9';
            case KEY_0 : return '0';
            case KEY_MINUS : return '-';
            case KEY_EQUAL : return '=';
            case KEY_SQUARE_OPEN_BRACKET : return '[';
            case KEY_SQUARE_CLOSE_BRACKET : return ']';
            case KEY_SEMICOLON : return ';';
            case KEY_BACKSLASH : return '\\';
            case KEY_COMMA : return ',';
            case KEY_DOT : return '.';
            case KEY_FORESLHASH : return '/';
            case KEY_SPACE : return ' ';
            case KEY_ENTER : return '\n';
            default : return 0;
        }
    }
}

char* str;
//char* curs;
//int count = 0;
char c[2] = "\0";

static void Keyboard_Handler(struct interrupt_registers *)
{
    uint8_t scancode = inb(0x60);
    switch (scancode)
    {
        case KEY_CAPS_LOCK:
            if (isCapsLock == false)
            {
                isCapsLock = true;
            }
            else
            {
                isCapsLock = false;
            }
            break;
        case KEY_L_SHIFT_P:
        case KEY_R_SHIFT_P:
            isShiftPressed = true;
            break;
        case KEY_L_SHIFT_R:
        case KEY_R_SHIFT_R:
            isShiftPressed = false;
            break;
        case KEY_ENTER:
            term_print("\n");
            for (int i = 0; i < strlen(str); i++)
            {
                str[i] = '\0';
            }
            break;
        case KEY_BACKSPACE:
            if (str[0] != '\0')
            {
                str[strlen(str) - 1] = '\0';
                term_print("\b \b");
            }
            break;
        case KEY_LEFT:
            cursor_left(1);
//            curs[count] = str[strlen(str) - (count + 1)];
//            count++;
            break;
        case KEY_RIGHT:
            cursor_right(1);
//            curs[count] = '\0';
//            count--;
            break;
        default:
            c[0] = get_ascii_char(scancode);
            term_print(c);
            strcat(str, c);
            break;
    }
}

void Keyboard_init()
{
    register_interrupt_handler(IRQ1, Keyboard_Handler);
    str[0] = '\0';
}
