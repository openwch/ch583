
#include "CH58xBLE_LIB.H"
#include <stdarg.h>
#include <string.h>
#include "log.h"
#include "CH58x_common.h"

#define ZEROPAD 1               // Pad with zero
#define SIGN    2               // Unsigned/signed long
#define PLUS    4               // Show plus
#define SPACE   8               // Space if plus
#define LEFT    16              // Left justified
#define SPECIAL 32              // 0x
#define LARGE   64              // Use 'ABCDEF' instead of 'abcdef'
#define is_digit(c) ((c) >= '0' && (c) <= '9')
static const char* digits = "0123456789abcdefghijklmnopqrstuvwxyz";
static const char* upper_digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";


static size_t _strnlen(const char* s, size_t count)
{
    const char* sc;

    for (sc = s; *sc != '\0' && count--; ++sc);

    return sc - s;
}
static int skip_atoi(const char** s)
{
    int i = 0;

    while (is_digit(**s)) i = i * 10 + *((*s)++) - '0';

    return i;
}
static void number(std_putc putc, long num, int base, int size, int precision, int type)
{
    char c, sign, tmp[66];
    const char* dig = digits;
    int i;
    char tmpch;

    if (type & LARGE)  dig = upper_digits;

    if (type & LEFT) type &= ~ZEROPAD;

    if (base < 2 || base > 36) return;

    c = (type & ZEROPAD) ? '0' : ' ';
    sign = 0;

    if (type & SIGN)
    {
        if (num < 0)
        {
            sign = '-';
            num = -num;
            size--;
        }
        else if (type & PLUS)
        {
            sign = '+';
            size--;
        }
        else if (type & SPACE)
        {
            sign = ' ';
            size--;
        }
    }

    if (type & SPECIAL)
    {
        if (base == 16)
            size -= 2;
        else if (base == 8)
            size--;
    }

    i = 0;

    if (num == 0)
        tmp[i++] = '0';
    else
    {
        while (num != 0)
        {
            tmp[i++] = dig[((unsigned long)num) % (unsigned)base];
            num = ((unsigned long)num) / (unsigned)base;
        }
    }

    if (i > precision) precision = i;

    size -= precision;

    if (!(type & (ZEROPAD | LEFT)))
    {
        while (size-- > 0)
        {
            tmpch = ' ';
            putc(&tmpch, 1);
        }
    }

    if (sign)
    {
        putc(&sign, 1);
    }

    if (type & SPECIAL)
    {
        if (base == 8)
        {
            tmpch = '0';
            putc(&tmpch, 1);
        }
        else if (base == 16)
        {
            tmpch = '0';
            putc(&tmpch, 1);
            tmpch = digits[33];
            putc(&tmpch, 1);
        }
    }

    if (!(type & LEFT))
    {
        while (size-- > 0)
        {
            putc(&c, 1);
        }
    }

    while (i < precision--)
    {
        tmpch = '0';
        putc(&tmpch, 1);
    }

    while (i-- > 0)
    {
        tmpch = tmp[i];
        putc(&tmpch, 1);
    }

    while (size-- > 0)
    {
        tmpch = ' ';
        putc(&tmpch, 1);
    }
}



static void log_vsprintf(std_putc putc, const char* fmt, va_list args)
{
    int len;
    unsigned long num;
    int base;
    char* s;
    int flags;            // Flags to number()
    int field_width;      // Width of output field
    int precision;        // Min. # of digits for integers; max number of chars for from string
    int qualifier;        // 'h', 'l', or 'L' for integer fields
    char* tmpstr = NULL;
    int tmpstr_size = 0;
    char tmpch;

    for (; *fmt; fmt++)
    {
        if (*fmt != '%')
        {
            if (tmpstr == NULL)
            {
                tmpstr = (char*)fmt;
                tmpstr_size = 0;
            }

            tmpstr_size ++;
            continue;
        }
        else if (tmpstr_size)
        {
            putc(tmpstr, tmpstr_size);
            tmpstr = NULL;
            tmpstr_size = 0;
        }

        // Process flags
        flags = 0;
repeat:
        fmt++; // This also skips first '%'

        switch (*fmt)
        {
        case '-':
            flags |= LEFT;
            goto repeat;

        case '+':
            flags |= PLUS;
            goto repeat;

        case ' ':
            flags |= SPACE;
            goto repeat;

        case '#':
            flags |= SPECIAL;
            goto repeat;

        case '0':
            flags |= ZEROPAD;
            goto repeat;
        }

        // Get field width
        field_width = -1;

        if (is_digit(*fmt))
            field_width = skip_atoi(&fmt);
        else if (*fmt == '*')
        {
            fmt++;
            field_width = va_arg(args, int);

            if (field_width < 0)
            {
                field_width = -field_width;
                flags |= LEFT;
            }
        }

        // Get the precision
        precision = -1;

        if (*fmt == '.')
        {
            ++fmt;

            if (is_digit(*fmt))
                precision = skip_atoi(&fmt);
            else if (*fmt == '*')
            {
                ++fmt;
                precision = va_arg(args, int);
            }

            if (precision < 0) precision = 0;
        }

        // Get the conversion qualifier
        qualifier = -1;

        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L')
        {
            qualifier = *fmt;
            fmt++;
        }

        // Default base
        base = 10;

        switch (*fmt)
        {

        case 'X':
            flags |= LARGE;

        case 'x':
            base = 16;
            break;

        default:
            if (*fmt != '%')
            {
                tmpch = '%';
                putc(&tmpch, 1);
            }

            if (*fmt)
            {
                tmpch = *fmt;
                putc(&tmpch, 1);
            }
            else
            {
                --fmt;
            }

            continue;
        }

        if (qualifier == 'l')
            num = va_arg(args, unsigned long);
        else if (qualifier == 'h')
        {
            if (flags & SIGN)
                num = va_arg(args, int);
            else
                num = va_arg(args, unsigned int);
        }
        else if (flags & SIGN)
            num = va_arg(args, int);
        else
            num = va_arg(args, unsigned int);

        number(putc, num, base, field_width, precision, flags);
    }

    if (tmpstr_size)
    {
        putc(tmpstr, tmpstr_size);
        tmpstr = NULL;
        tmpstr_size = 0;
    }
}

static void _uart_putc(char* data, uint16_t size)
{
  int i;
  for(i=0; i<size; i++)
  {
    while( R8_UART1_TFC == UART_FIFO_SIZE );                        /* 等待数据发送 */
    R8_UART1_THR = *data++;                                               /* 发送数据 */
  }
}

void dbg_printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    log_vsprintf(_uart_putc, format, args);
    va_end(args);
}

void my_dump_byte(uint8_t* pData, int dlen)
{
    for(int i=0; i<dlen; i++)
    {
        dbg_printf("%02x ",pData[i]);
    }

    dbg_printf("\n");
}


