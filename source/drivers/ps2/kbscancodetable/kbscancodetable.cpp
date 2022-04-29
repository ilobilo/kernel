// Copyright (C) 2021-2022  ilobilo

#include <drivers/ps2/kbscancodetable/kbscancodetable.hpp>

// Following code is from https://github.com/torvalds/linux/blob/master/drivers/tty/vt/defkeymap.c_shipped

uint16_t plain_map[NUM_KEYS]
{
    0xF200, 0xF01B, 0xF031, 0xF032, 0xF033, 0xF034, 0xF035, 0xF036,
    0xF037, 0xF038, 0xF039, 0xF030, 0xF02D, 0xF03D, 0xF07F, 0xF009,
    0xFB71, 0xFB77, 0xFB65, 0xFB72, 0xFB74, 0xFB79, 0xFB75, 0xFB69,
    0xFB6F, 0xFB70, 0xF05B, 0xF05D, 0xF201, 0xF702, 0xFB61, 0xFB73,
    0xFB64, 0xFB66, 0xFB67, 0xFB68, 0xFB6A, 0xFB6B, 0xFB6C, 0xF03B,
    0xF027, 0xF060, 0xF700, 0xF05C, 0xFB7A, 0xFB78, 0xFB63, 0xFB76,
    0xFB62, 0xFB6E, 0xFB6D, 0xF02C, 0xF02E, 0xF02F, 0xF700, 0xF30C,
    0xF703, 0xF020, 0xF207, 0xF100, 0xF101, 0xF102, 0xF103, 0xF104,
    0xF105, 0xF106, 0xF107, 0xF108, 0xF109, 0xF208, 0xF209, 0xF307,
    0xF308, 0xF309, 0xF30B, 0xF304, 0xF305, 0xF306, 0xF30A, 0xF301,
    0xF302, 0xF303, 0xF300, 0xF310, 0xF206, 0xF200, 0xF03C, 0xF10A,
    0xF10B, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200,
    0xF30E, 0xF702, 0xF30D, 0xF01C, 0xF701, 0xF205, 0xF114, 0xF603,
    0xF118, 0xF601, 0xF602, 0xF117, 0xF600, 0xF119, 0xF115, 0xF116,
    0xF11A, 0xF10C, 0xF10D, 0xF11B, 0xF11C, 0xF110, 0xF311, 0xF11D,
    0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200,
};

uint16_t shift_map[NUM_KEYS]
{
    0xF200, 0xF01B, 0xF021, 0xF040, 0xF023, 0xF024, 0xF025, 0xF05E,
    0xF026, 0xF02A, 0xF028, 0xF029, 0xF05F, 0xF02B, 0xF07F, 0xF009,
    0xFB51, 0xFB57, 0xFB45, 0xFB52, 0xFB54, 0xFB59, 0xFB55, 0xFB49,
    0xFB4F, 0xFB50, 0xF07B, 0xF07D, 0xF201, 0xF702, 0xFB41, 0xFB53,
    0xFB44, 0xFB46, 0xFB47, 0xFB48, 0xFB4A, 0xFB4B, 0xFB4C, 0xF03A,
    0xF022, 0xF07E, 0xF700, 0xF07C, 0xFB5A, 0xFB58, 0xFB43, 0xFB56,
    0xFB42, 0xFB4E, 0xFB4D, 0xF03C, 0xF03E, 0xF03F, 0xF700, 0xF30C,
    0xF703, 0xF020, 0xF207, 0xF10A, 0xF10B, 0xF10C, 0xF10D, 0xF10E,
    0xF10F, 0xF110, 0xF111, 0xF112, 0xF113, 0xF213, 0xF203, 0xF307,
    0xF308, 0xF309, 0xF30B, 0xF304, 0xF305, 0xF306, 0xF30A, 0xF301,
    0xF302, 0xF303, 0xF300, 0xF310, 0xF206, 0xF200, 0xF03E, 0xF10A,
    0xF10B, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200,
    0xF30E, 0xF702, 0xF30D, 0xF200, 0xF701, 0xF205, 0xF114, 0xF603,
    0xF20B, 0xF601, 0xF602, 0xF117, 0xF600, 0xF20A, 0xF115, 0xF116,
    0xF11A, 0xF10C, 0xF10D, 0xF11B, 0xF11C, 0xF110, 0xF311, 0xF11D,
    0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200,
};

uint16_t altgr_map[NUM_KEYS]
{
    0xF200, 0xF200, 0xF200, 0xF040, 0xF200, 0xF024, 0xF200, 0xF200,
    0xF07B, 0xF05B, 0xF05D, 0xF07D, 0xF05C, 0xF200, 0xF200, 0xF200,
    0xFB71, 0xFB77, 0xF918, 0xFB72, 0xFB74, 0xFB79, 0xFB75, 0xFB69,
    0xFB6F, 0xFB70, 0xF200, 0xF07E, 0xF201, 0xF702, 0xF914, 0xFB73,
    0xF917, 0xF919, 0xFB67, 0xFB68, 0xFB6A, 0xFB6B, 0xFB6C, 0xF200,
    0xF200, 0xF200, 0xF700, 0xF200, 0xFB7A, 0xFB78, 0xF916, 0xFB76,
    0xF915, 0xFB6E, 0xFB6D, 0xF200, 0xF200, 0xF200, 0xF700, 0xF30C,
    0xF703, 0xF200, 0xF207, 0xF50C, 0xF50D, 0xF50E, 0xF50F, 0xF510,
    0xF511, 0xF512, 0xF513, 0xF514, 0xF515, 0xF208, 0xF202, 0xF911,
    0xF912, 0xF913, 0xF30B, 0xF90E, 0xF90F, 0xF910, 0xF30A, 0xF90B,
    0xF90C, 0xF90D, 0xF90A, 0xF310, 0xF206, 0xF200, 0xF07C, 0xF516,
    0xF517, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200,
    0xF30E, 0xF702, 0xF30D, 0xF200, 0xF701, 0xF205, 0xF114, 0xF603,
    0xF118, 0xF601, 0xF602, 0xF117, 0xF600, 0xF119, 0xF115, 0xF116,
    0xF11A, 0xF10C, 0xF10D, 0xF11B, 0xF11C, 0xF110, 0xF311, 0xF11D,
    0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200,
};

uint16_t ctrl_map[NUM_KEYS]
{
    0xF200, 0xF200, 0xF200, 0xF000, 0xF01B, 0xF01C, 0xF01D, 0xF01E,
    0xF01F, 0xF07F, 0xF200, 0xF200, 0xF01F, 0xF200, 0xF008, 0xF200,
    0xF011, 0xF017, 0xF005, 0xF012, 0xF014, 0xF019, 0xF015, 0xF009,
    0xF00F, 0xF010, 0xF01B, 0xF01D, 0xF201, 0xF702, 0xF001, 0xF013,
    0xF004, 0xF006, 0xF007, 0xF008, 0xF00A, 0xF00B, 0xF00C, 0xF200,
    0xF007, 0xF000, 0xF700, 0xF01C, 0xF01A, 0xF018, 0xF003, 0xF016,
    0xF002, 0xF00E, 0xF00D, 0xF200, 0xF20E, 0xF07F, 0xF700, 0xF30C,
    0xF703, 0xF000, 0xF207, 0xF100, 0xF101, 0xF102, 0xF103, 0xF104,
    0xF105, 0xF106, 0xF107, 0xF108, 0xF109, 0xF208, 0xF204, 0xF307,
    0xF308, 0xF309, 0xF30B, 0xF304, 0xF305, 0xF306, 0xF30A, 0xF301,
    0xF302, 0xF303, 0xF300, 0xF310, 0xF206, 0xF200, 0xF200, 0xF10A,
    0xF10B, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200,
    0xF30E, 0xF702, 0xF30D, 0xF01C, 0xF701, 0xF205, 0xF114, 0xF603,
    0xF118, 0xF601, 0xF602, 0xF117, 0xF600, 0xF119, 0xF115, 0xF116,
    0xF11A, 0xF10C, 0xF10D, 0xF11B, 0xF11C, 0xF110, 0xF311, 0xF11D,
    0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200,
};

uint16_t shift_ctrl_map[NUM_KEYS]
{
    0xF200, 0xF200, 0xF200, 0xF000, 0xF200, 0xF200, 0xF200, 0xF200,
    0xF200, 0xF200, 0xF200, 0xF200, 0xF01F, 0xF200, 0xF200, 0xF200,
    0xF011, 0xF017, 0xF005, 0xF012, 0xF014, 0xF019, 0xF015, 0xF009,
    0xF00F, 0xF010, 0xF200, 0xF200, 0xF201, 0xF702, 0xF001, 0xF013,
    0xF004, 0xF006, 0xF007, 0xF008, 0xF00A, 0xF00B, 0xF00C, 0xF200,
    0xF200, 0xF200, 0xF700, 0xF200, 0xF01A, 0xF018, 0xF003, 0xF016,
    0xF002, 0xF00E, 0xF00D, 0xF200, 0xF200, 0xF200, 0xF700, 0xF30C,
    0xF703, 0xF200, 0xF207, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200,
    0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF208, 0xF200, 0xF307,
    0xF308, 0xF309, 0xF30B, 0xF304, 0xF305, 0xF306, 0xF30A, 0xF301,
    0xF302, 0xF303, 0xF300, 0xF310, 0xF206, 0xF200, 0xF200, 0xF200,
    0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200,
    0xF30E, 0xF702, 0xF30D, 0xF200, 0xF701, 0xF205, 0xF114, 0xF603,
    0xF118, 0xF601, 0xF602, 0xF117, 0xF600, 0xF119, 0xF115, 0xF116,
    0xF11A, 0xF10C, 0xF10D, 0xF11B, 0xF11C, 0xF110, 0xF311, 0xF11D,
    0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200,
};

uint16_t alt_map[NUM_KEYS]
{
    0xF200, 0xF81B, 0xF831, 0xF832, 0xF833, 0xF834, 0xF835, 0xF836,
    0xF837, 0xF838, 0xF839, 0xF830, 0xF82D, 0xF83D, 0xF87F, 0xF809,
    0xF871, 0xF877, 0xF865, 0xF872, 0xF874, 0xF879, 0xF875, 0xF869,
    0xF86F, 0xF870, 0xF85B, 0xF85D, 0xF80D, 0xF702, 0xF861, 0xF873,
    0xF864, 0xF866, 0xF867, 0xF868, 0xF86A, 0xF86B, 0xF86C, 0xF83B,
    0xF827, 0xF860, 0xF700, 0xF85C, 0xF87A, 0xF878, 0xF863, 0xF876,
    0xF862, 0xF86E, 0xF86D, 0xF82C, 0xF82E, 0xF82F, 0xF700, 0xF30C,
    0xF703, 0xF820, 0xF207, 0xF500, 0xF501, 0xF502, 0xF503, 0xF504,
    0xF505, 0xF506, 0xF507, 0xF508, 0xF509, 0xF208, 0xF209, 0xF907,
    0xF908, 0xF909, 0xF30B, 0xF904, 0xF905, 0xF906, 0xF30A, 0xF901,
    0xF902, 0xF903, 0xF900, 0xF310, 0xF206, 0xF200, 0xF83C, 0xF50A,
    0xF50B, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200,
    0xF30E, 0xF702, 0xF30D, 0xF01C, 0xF701, 0xF205, 0xF114, 0xF603,
    0xF118, 0xF210, 0xF211, 0xF117, 0xF600, 0xF119, 0xF115, 0xF116,
    0xF11A, 0xF10C, 0xF10D, 0xF11B, 0xF11C, 0xF110, 0xF311, 0xF11D,
    0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200,
};

uint16_t ctrl_alt_map[NUM_KEYS]
{
    0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200,
    0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200,
    0xF811, 0xF817, 0xF805, 0xF812, 0xF814, 0xF819, 0xF815, 0xF809,
    0xF80F, 0xF810, 0xF200, 0xF200, 0xF201, 0xF702, 0xF801, 0xF813,
    0xF804, 0xF806, 0xF807, 0xF808, 0xF80A, 0xF80B, 0xF80C, 0xF200,
    0xF200, 0xF200, 0xF700, 0xF200, 0xF81A, 0xF818, 0xF803, 0xF816,
    0xF802, 0xF80E, 0xF80D, 0xF200, 0xF200, 0xF200, 0xF700, 0xF30C,
    0xF703, 0xF200, 0xF207, 0xF500, 0xF501, 0xF502, 0xF503, 0xF504,
    0xF505, 0xF506, 0xF507, 0xF508, 0xF509, 0xF208, 0xF200, 0xF307,
    0xF308, 0xF309, 0xF30B, 0xF304, 0xF305, 0xF306, 0xF30A, 0xF301,
    0xF302, 0xF303, 0xF300, 0xF20C, 0xF206, 0xF200, 0xF200, 0xF50A,
    0xF50B, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200,
    0xF30E, 0xF702, 0xF30D, 0xF200, 0xF701, 0xF205, 0xF114, 0xF603,
    0xF118, 0xF601, 0xF602, 0xF117, 0xF600, 0xF119, 0xF115, 0xF20C,
    0xF11A, 0xF10C, 0xF10D, 0xF11B, 0xF11C, 0xF110, 0xF311, 0xF11D,
    0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200, 0xF200,
};

uint16_t *key_maps[MAX_NUM_KEYMAPS]
{
	plain_map, shift_map, altgr_map, nullptr,
	ctrl_map, shift_ctrl_map, nullptr, nullptr,
	alt_map, nullptr, nullptr, nullptr,
	ctrl_alt_map, ctrl_alt_map
};

unsigned int keymap_count = 7;

static char func_buf[]
{
    '\033', '[', '[', 'A', 0,
    '\033', '[', '[', 'B', 0,
    '\033', '[', '[', 'C', 0,
    '\033', '[', '[', 'D', 0,
    '\033', '[', '[', 'E', 0,
    '\033', '[', '1', '7', '~', 0,
    '\033', '[', '1', '8', '~', 0,
    '\033', '[', '1', '9', '~', 0,
    '\033', '[', '2', '0', '~', 0,
    '\033', '[', '2', '1', '~', 0,
    '\033', '[', '2', '3', '~', 0,
    '\033', '[', '2', '4', '~', 0,
    '\033', '[', '2', '5', '~', 0,
    '\033', '[', '2', '6', '~', 0,
    '\033', '[', '2', '8', '~', 0,
    '\033', '[', '2', '9', '~', 0,
    '\033', '[', '3', '1', '~', 0,
    '\033', '[', '3', '2', '~', 0,
    '\033', '[', '3', '3', '~', 0,
    '\033', '[', '3', '4', '~', 0,
    '\033', '[', '1', '~', 0,
    '\033', '[', '2', '~', 0,
    '\033', '[', '3', '~', 0,
    '\033', '[', '4', '~', 0,
    '\033', '[', '5', '~', 0,
    '\033', '[', '6', '~', 0,
    '\033', '[', 'M', 0,
    '\033', '[', 'P', 0
};

char *func_table[MAX_NUM_FUNC]
{
    func_buf + 0,
    func_buf + 5,
    func_buf + 10,
    func_buf + 15,
    func_buf + 20,
    func_buf + 25,
    func_buf + 31,
    func_buf + 37,
    func_buf + 43,
    func_buf + 49,
    func_buf + 55,
    func_buf + 61,
    func_buf + 67,
    func_buf + 73,
    func_buf + 79,
    func_buf + 85,
    func_buf + 91,
    func_buf + 97,
    func_buf + 103,
    func_buf + 109,
    func_buf + 115,
    func_buf + 120,
    func_buf + 125,
    func_buf + 130,
    func_buf + 135,
    func_buf + 140,
    func_buf + 145,
    nullptr,
    nullptr,
    func_buf + 149,
    nullptr,
};