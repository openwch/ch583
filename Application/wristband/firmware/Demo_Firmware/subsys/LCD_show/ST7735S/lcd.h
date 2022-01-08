#ifndef __LCD_H
#define __LCD_H		
#include "CH58x_common.h"

void LCD_Fill(uint16_t xsta,uint16_t ysta,uint16_t xend,uint16_t yend,uint16_t color);//指定区域填充颜色
void LCD_DrawPoint(uint16_t x,uint16_t y,uint16_t color);//在指定位置画一个点
void LCD_DrawLine(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t color);//在指定位置画一条线
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,uint16_t color);//在指定位置画一个矩形
void Draw_Circle(uint16_t x0,uint16_t y0,uint8_t r,uint16_t color);//在指定位置画一个圆

void LCD_ShowChinese(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);//显示汉字串
void LCD_ShowChinese12x12(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);//显示单个12x12汉字
void LCD_ShowChinese16x16(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);//显示单个16x16汉字
void LCD_ShowChinese24x24(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);//显示单个24x24汉字
void LCD_ShowChinese32x32(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);//显示单个32x32汉字

void LCD_ShowChar(uint16_t x,uint16_t y,uint8_t num,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);//显示一个字符
void LCD_ShowString(uint16_t x,uint16_t y,const uint8_t *p,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);//显示字符串
uint32_t mypow(uint8_t m,uint8_t n);//求幂
void LCD_ShowIntNum0(uint16_t x,uint16_t y,uint16_t num,uint8_t len,uint16_t fc,uint16_t bc,uint8_t sizey, uint8_t mode);//显示整数变量
void LCD_ShowIntNum(uint16_t x,uint16_t y,uint16_t num,uint8_t len,uint16_t fc,uint16_t bc,uint8_t sizey, uint8_t mode);//显示整数变量
void LCD_ShowFloatNum1(uint16_t x,uint16_t y,float num,uint8_t len,uint16_t fc,uint16_t bc,uint8_t sizey);//显示两位小数变量

void LCD_ShowPicture(uint16_t x,uint16_t y,uint16_t length,uint16_t width,const uint8_t pic[]);//显示图片



void LCD_Showtest(uint16_t x,uint16_t y,uint16_t length,uint16_t width);

//画笔颜色
//#define WHITE         	 0xFFFF
//#define BLACK         	 0x0000
//#define BLUE           	 0x001F
//#define BRED             0XF81F
//#define GRED 			 0XFFE0
//#define GBLUE			 0X07FF
//#define RED           	 0xF800
//#define MAGENTA       	 0xF81F
//#define GREEN         	 0x07E0
//#define CYAN          	 0x7FFF
//#define YELLOW        	 0xFFE0
//#define BROWN 			 0XBC40 //棕色
//#define BRRED 			 0XFC07 //棕红色
//#define GRAY  			 0X8430 //灰色
//#define DARKBLUE      	 0X01CF	//深蓝色
//#define LIGHTBLUE      	 0X7D7C	//浅蓝色
//#define GRAYBLUE       	 0X5458 //灰蓝色
//#define LIGHTGREEN     	 0X841F //浅绿色
//#define LGRAY 			 0XC618 //浅灰色(PANNEL),窗体背景色
//#define LGRAYBLUE        0XA651 //浅灰蓝色(中间层颜色)
//#define LBBLUE           0X2B12 //浅棕蓝色(选择条目的反色)




#define BLACK    0X0
#define DIMGRAY    0X6B4D
#define GRAY    0X8410
#define DARK_GRAY    0XAD55
#define SILVER    0XC618
#define LIGHT_GRAY    0XD69A
#define GAINSBORO    0XDEFB
#define WHITE_SMOKE    0XF7BE
#define WHITE    0XFFFF
#define SNOW    0XFFDF
#define IRON_GRAY    0X62CA
#define SAND_BEIGE    0XE618
#define ROSY_BROWN    0XBC71
#define LIGHT_CORAL    0XF410
#define INDIAN_RED    0XCAEB
#define BROWN    0XA145
#define FIRE_BRICK    0XB104
#define MAROON    0X8000
#define DARK_RED    0X8800
#define STRONG_RED    0XE000
#define RED    0XF800
#define PERSIMMON    0XFA68
#define MISTY_ROSE    0XFF3C
#define SALMON    0XFC0E
#define SCARLET    0XF920
#define TOMATO    0XFB08
#define DARK_SALMON    0XECAF
#define CORAL    0XFBEA
#define ORANGE_RED    0XFA20
#define LIGHT_SALMON    0XFD0F
#define VERMILION    0XFA60
#define SIENNA    0XA285
#define TROPICAL_ORANGE    0XFC06
#define CAMEL    0XA348
#define APRICOT    0XE4CC
#define COCONUT_BROWN    0X48E0
#define SEASHELL    0XFFBD
#define SADDLE_BROWN    0X8A22
#define CHOCOLATE    0XD343
#define BURNT_ORANGE    0XCAA0
#define SUN_ORANGE    0XFB80
#define PEACH_PUFF    0XFED7
#define SAND_BROWN    0XF52C
#define BRONZE    0XBB86
#define LINEN    0XFF9C
#define HONEY_ORANGE    0XFD8C
#define PERU    0XCC27
#define SEPIA    0X7202
#define OCHER    0XCBA4
#define BISQUE    0XFF38
#define TANGERINE    0XF420
#define DARK_ORANGE    0XFC60
#define ANTIQUE_WHITE    0XFF5A
#define TAN    0XD5B1
#define BURLY_WOOD    0XDDD0
#define BLANCHED_ALMOND    0XFF59
#define NAVAJO_WHITE    0XFEF5
#define MARIGOLD    0XFCC0
#define PAPAYA_WHIP    0XFF7A
#define PALE_OCRE    0XCD91
#define KHAKI    0X9B43
#define MOCCASIN    0XFF36
#define OLD_LACE    0XFFBC
#define WHEAT    0XF6F6
#define PEACH    0XFF36
#define ORANGE    0XFD20
#define FLORAL_WHITE    0XFFDE
#define GOLDENROD    0XDD24
#define DARK_GOLDENROD    0XBC21
#define COFFEE    0X49C0
#define JASMINE    0XE60B
#define AMBER    0XFDE0
#define CORNSILK    0XFFDB
#define CHROME_YELLOW    0XE5C0
#define GOLDEN    0XFEA0
#define LEMON_CHIFFON    0XFFD9
#define LIGHT_KHAKI    0XF731
#define PALE_GOLDENROD    0XEF55
#define DARK_KHAKI    0XBDAD
#define MIMOSA    0XE6C6
#define CREAM    0XFFFA
#define IVORY    0XFFFE
#define BEIGE    0XF7BB
#define LIGHT_YELLOW    0XFFFC
#define LIGHT_GOLDENROD_YELLOW    0XFFDA
#define CHAMPAGNE_YELLOW    0XFFF3
#define MUSTARD    0XCE69
#define MOON_YELLOW    0XFFE9
#define OLIVE    0X8400
#define CANARY_YELLOW    0XFFE0
#define YELLOW    0XFFE0
#define MOSS_GREEN    0X6BA4
#define LIGHT_LIME    0XCFE0
#define OLIVE_DRAB    0X6C64
#define YELLOW_GREEN    0X9E66
#define DARK_OLIVE_GREEN    0X5345
#define APPLE_GREEN    0X8F20
#define GREEN_YELLOW    0XAFE5
#define GRASS_GREEN    0X9F29
#define LAWN_GREEN    0X7FE0
#define CHARTREUSE    0X7FE0
#define FOLIAGE_GREEN    0X75C7
#define FRESH_LEAVES    0X9FE9
#define BRIGHT_GREEN    0X67E0
#define COBALT_GREEN    0X67EB
#define HONEYDEW    0XF7FE
#define DARK_SEA_GREEN    0X8DF1
#define LIGHT_GREEN    0X9772
#define PALE_GREEN    0X9FD3
#define IVY_GREEN    0X35E6
#define FOREST_GREEN    0X2444
#define LIME_GREEN    0X3666
#define DARK_GREEN    0X320
#define GREEN    0X400
#define LIME    0X7E0
#define MALACHITE    0X2605
#define MINT    0X14C5
#define CELADON_GREEN    0X7731
#define EMERALD    0X564F
#define TURQUOISE_GREEN    0X4F30
#define VERIDIAN    0X13A6
#define HORIZON_BLUE    0XA7F9
#define SEA_GREEN    0X2C4A
#define MEDIUM_SEA_GREEN    0X3D8E
#define MINT_CREAM    0XF7FF
#define SPRING_GREEN    0X7F0
#define PEACOCK_GREEN    0X50B
#define MEDIUM_SPRING_GREEN    0X7D3
#define MEDIUM_AQUAMARINE    0X6675
#define AQUAMARINE    0X7FFA
#define CYAN_BLUE    0XDF1
#define AQUA_BLUE    0X67FC
#define TURQUOISE_BLUE    0X3739
#define TURQUOISE    0X36B9
#define LIGHT_SEA_GREEN    0X2595
#define MEDIUM_TURQUOISE    0X4E99
#define LIGHT_CYAN    0XE7FF
#define BABY_BLUE    0XE7FF
#define PALE_TURQUOISE    0XAF7D
#define DARK_SLATE_GRAY    0X2A69
#define TEAL    0X410
#define DARK_CYAN    0X451
#define CYAN    0X7FF
#define AQUA    0XAEFC
#define DARK_TURQUOISE    0X67A
#define CADET_BLUE    0X5CF4
#define PEACOCK_BLUE    0X411
#define POWDER_BLUE    0XB71C
#define STRONG_BLUE    0X30E
#define LIGHT_BLUE    0XAEDC
#define PALE_BLUE    0X7DD9
#define SAXE_BLUE    0X44D6
#define DEEP_SKY_BLUE    0X5FF
#define SKY_BLUE    0X867D
#define LIGHT_SKY_BLUE    0X867F
#define MARINE_BLUE    0X22F
#define PRUSSIAN_BLUE    0X18A
#define STEEL_BLUE    0X4416
#define ALICE_BLUE    0XF7DF
#define SLATE_GRAY    0X7412
#define LIGHT_SLATE_GRAY    0X7453
#define DODGER_BLUE    0X1C9F
#define MINERAL_BLUE    0X273
#define AZURE    0X3FF
#define WEDGWOOD_BLUE    0X5437
#define LIGHT_STEEL_BLUE    0XB63B
#define COBALT_BLUE    0X235
#define PALE_DENIM    0X5C38
#define CORNFLOWER_BLUE    0X64BD
#define SALVIA_BLUE    0X4C1C
#define DARK_POWDER_BLUE    0X193
#define SAPPHIRE    0X92C
#define INTERNATIONAL_KLEIN_BLUE    0X174
#define CERULEAN_BLUE    0X2A97
#define ROYAL_BLUE    0X435C
#define DARK_MINERAL_BLUE    0X21AF
#define ULTRAMARINE    0X19F
#define LAPIS_LAZULI    0X99F
#define GHOST_WHITE    0XFFDF
#define LAVENDER    0XE73F
#define PERIWINKLE    0XCE7F
#define MIDNIGHT_BLUE    0X18CE
#define NAVY_BLUE    0X10
#define DARK_BLUE    0X11
#define MEDIUM_BLUE    0X19
#define BLUE    0X1F
#define WISTERIA    0X5A9C
#define DARK_SLATE_BLUE    0X49F1
#define SLATE_BLUE    0X6AD9
#define MEDIUM_SLATE_BLUE    0X7B5D
#define MAUVE    0X621F
#define LILAC    0XB4DF
#define MEDIUM_PURPLE    0X939B
#define AMETHYST    0X6199
#define GRAYISH_PURPLE    0X83B4
#define HELIOTROPE    0X5017
#define MINERAL_VIOLET    0XBD19
#define BLUE_VIOLET    0X895C
#define VIOLET    0X881F
#define INDIGO    0X4810
#define DARK_ORCHID    0X9999
#define DARK_VIOLET    0X901A
#define PANSY    0X7014
#define MALLOW    0XDA7F
#define OPERA_MAUVE    0XE41F
#define MEDIUM_ORCHID    0XBABA
#define PAIL_LILAC    0XE67C
#define THISTLE    0XDDFB
#define CLEMATIS    0XCD19
#define PLUM    0XDD1B
#define LIGHT_VIOLET    0XEC1D
#define PURPLE    0X8010
#define DARK_MAGENTA    0X8811
#define MAGENTA    0XF81F
#define FUCHSIA    0XF014
#define ORCHID    0XDB9A
#define PEARL_PINK    0XFD9C
#define OLD_ROSE    0XBAB3
#define ROSE_PINK    0XFB39
#define MEDIUM_VIOLET_RED    0XC0B0
#define MAGENTA_ROSE    0XF874
#define ROSE    0XF80F
#define RUBY    0XC810
#define CAMELLIA    0XE1D2
#define DEEP_PINK    0XF8B2
#define FLAMINGO    0XE457
#define CORAL_PINK    0XFC17
#define HOT_PINK    0XFB56
#define BURGUNDY    0X4004
#define SPINEL_RED    0XFB96
#define CARMINE    0XE00B
#define BABY_PINK    0XFEDC
#define CARDINAL_RED    0X9806
#define LAVENDER_BLUSH    0XFF9E
#define PALE_VIOLET_RED    0XDB92
#define CERISE    0XD98C
#define SALMON_PINK    0XFC13
#define CRIMSON    0XD8A7
#define PINK    0XFE19
#define LIGHT_PINK    0XFDB8
#define SHELL_PINK    0XFD97
#define ALIZARIN_CRIMSON    0XE126











#endif





