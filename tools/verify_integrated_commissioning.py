#!/usr/bin/env python3
"""Exercise real diagnostic conversion code and both physical-board profiles."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "firmware/ac-integrated-commissioning-esp32s3"


def verify():
    cc = shutil.which("cc")
    if not cc:
        raise RuntimeError("A local C compiler is required for the bounded host checks")
    results = []
    with tempfile.TemporaryDirectory(prefix="ac-integrated-check-") as name:
        work = Path(name)
        math = work / "math.c"
        math.write_text(r'''
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include "diagnostic_math.h"
int main(void) {
    for (int32_t n = INT16_MIN; n <= INT16_MAX; ++n) {
        assert(ac_adc_microvolts((int16_t)n) == n * 125);
        uint16_t value = (uint16_t)n;
        const uint8_t bytes[] = {(uint8_t)(value >> 8), (uint8_t)value};
        assert(ac_word(bytes, false) == value);
        assert(ac_word(bytes, true) == (uint16_t)((value << 8) | (value >> 8)));
    }
    const uint16_t expected[] = {0xc383,0xd383,0xe383,0xf383};
    for (unsigned n = 0; n < 4; ++n) {
        uint16_t actual = 0;
        assert(ac_ads_configuration(n, &actual));
        assert(actual == expected[n]);
    }
    uint16_t unchanged = 0x55aa;
    assert(!ac_ads_configuration(4, &unchanged));
    assert(!ac_ads_configuration(UINT_MAX, &unchanged));
    assert(unchanged == 0x55aa);
    assert(!ac_ads_configuration(0, NULL));
    return 0;
}
''')
        subprocess.run([cc,"-std=c11","-Wall","-Wextra","-Werror","-I",str(SOURCE/"main"),
            str(math),str(SOURCE/"main/diagnostic_math.c"),"-o",str(work/"math")],check=True)
        subprocess.run([str(work/"math")],check=True,timeout=10)
        results.append("All 65,536 ADC codes, both byte orders and ADS1115 mode/rejection cases")
        profile = work / "profile.c"
        profile.write_text(r'''
#include <stdio.h>
#include "board.h"
int main(void) {
    printf("{\"board\":\"%s\",\"ads\":%u,\"gauge\":%u,\"display\":%u,\"gpio\":["
           "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u]}",
           AC_BOARD_NAME, AC_ADS_ADDRESS, AC_GAUGE_ADDRESS, AC_HAS_DISPLAY,
           AC_PRESS_H_ON,AC_PRESS_L_ON,AC_BOOST_EN,AC_ADC_RDY,AC_SDA,AC_SCL,AC_MUX,AC_SD_CD,
           AC_SD_CS,AC_MOSI,AC_SCK,AC_MISO,AC_PRESS_H_FLT,AC_PRESS_L_FLT,
           AC_MENU,AC_UP,AC_DOWN,AC_SELECT,AC_LCD_CS,AC_LCD_DC,AC_LCD_RESET,AC_LCD_BL,
           AC_TOUCH_IRQ,AC_RTD_H_CS);
    return 0;
}
''')
        compiled = {}
        # This independent expectation is the actual native schematic/module-pad audit.
        gpio = [4,5,6,7,15,16,17,18,9,10,11,12,21,47,8,48,2,1,13,14,38,39,40,41]
        for board,address,display in (("H2",0x49,1),("R9",0x48,0)):
            binary = work/board
            subprocess.run([cc,"-std=c11","-Wall","-Wextra","-Werror","-D",f"AC_BOARD_{board}=1",
                "-I",str(SOURCE/"main"),str(profile),"-o",str(binary)],check=True)
            actual = json.loads(subprocess.check_output([str(binary)],text=True))
            expected = {"board":board,"ads":address,"gauge":0x36,"display":display,"gpio":gpio}
            if actual != expected:
                raise ValueError(f"The compiled {board} profile does not match the native electrical audit")
            compiled[board] = actual
        for definitions in ([],["-DAC_BOARD_H2=1","-DAC_BOARD_R9=1"]):
            failed = subprocess.run([cc,"-std=c11",*definitions,"-I",str(SOURCE/"main"),str(profile),
                "-o",str(work/"invalid")],capture_output=True)
            if failed.returncode == 0:
                raise ValueError("An absent or ambiguous physical board was accepted")
        results.append("Both compiled board profiles match native GPIO/I2C expectations; ambiguous boards fail")
    return {"checks":results,"compiled_profiles":compiled,"physical_tested":False}


if __name__ == "__main__":
    print(json.dumps(verify(),indent=2))
