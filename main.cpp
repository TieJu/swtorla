#include "app.h"
#include <stdexcept>
#include <Windows.h>
#include <shellapi.h>

#define APP_CATION "SW:ToR Log Analizer"
#define CONFIG_PATH "./config.json"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "swtor_log_parser.h"

void test_parser() {
    const char* lines[] =
    {"[19:34:01.936] [@Crimsonia] [Dash'Roode {3153558262251520}:1589000694342] [Explosive Dart {2288745122365440}] [RemoveEffect {836045448945478}: Explosive Dart {2288745122365440}] ()"
,"[19:34:02.016] [@Crimsonia] [Dash'Roode {3153558262251520}:1589000694342] [Immolate {2030175206244352}] [ApplyEffect {836045448945477}: Damage {836045448945501}] (7230* elemental {836045448940875}) <7230>"
,"[19:34:02.354] [@Crimsonia] [Dash'Roode {3153558262251520}:1589000694342] [Bleeding {2029831608860928}] [ApplyEffect {836045448945477}: Damage {836045448945501}] (305 internal {836045448940876}) <305>"
,"[19:34:02.569] [@Crimsonia] [@Crimsonia] [Shoulder Cannon {3066482095292416}] [Event {836045448945472}: AbilityActivate {836045448945479}] ()"
,"[19:34:03.383] [@Crimsonia] [Dash'Roode {3153558262251520}:1589000694342] [Shoulder Cannon {3066494980194584}] [ApplyEffect {836045448945477}: Damage {836045448945501}] (1554 kinetic {836045448940873}) <1554>"
,"[19:34:03.384] [@Crimsonia] [Dash'Roode {3153558262251520}:1589000694342] [Bleeding {2029831608860928}] [ApplyEffect {836045448945477}: Damage {836045448945501}] (618* internal {836045448940876}) <618>"
,"[19:34:03.384] [@Crimsonia] [@Crimsonia] [Rocket Punch {814300029517824}] [Event {836045448945472}: AbilityActivate {836045448945479}] ()"
,"[19:34:03.385] [@Crimsonia] [@Crimsonia] [Flame Barrage {2035664174448640}] [RemoveEffect {836045448945478}: Flame Barrage {2035664174448640}] ()"
,"[19:34:03.385] [@Crimsonia] [@Crimsonia] [Charged Gauntlets {2036029246668800}] [ApplyEffect {836045448945477}: Charged Gauntlets {2036029246669056}] ()"
,"[19:34:03.490] [@Crimsonia] [Dash'Roode {3153558262251520}:1589000694342] [Rocket Punch {814300029517824}] [ApplyEffect {836045448945477}: Damage {836045448945501}] (5566* kinetic {836045448940873}) <5566>"
,"[19:34:04.374] [@Crimsonia] [Dash'Roode {3153558262251520}:1589000694342] [Bleeding {2029831608860928}] [ApplyEffect {836045448945477}: Damage {836045448945501}] (618* internal {836045448940876}) <618>"
,"[19:34:04.420] [@Hesu] [@Crimsonia] [Kolto Missile {3163922018336768}] [ApplyEffect {836045448945477}: Heal {836045448945500}] (4002*) <1801>"
,"[19:34:05.007] [@Crimsonia] [@Crimsonia] [Rail Shot {814278554681344}] [Event {836045448945472}: AbilityActivate {836045448945479}] ()"
,"[19:34:05.007] [@Crimsonia] [@Crimsonia] [] [Spend {836045448945473}: energy {836045448938503}] (16)"
,"[19:34:05.118] [@Hesu] [@Crimsonia] [Kolto Missile {3163922018336768}] [ApplyEffect {836045448945477}: Heal {836045448945500}] (633*) <284>"
,"[19:34:05.119] [@Crimsonia] [@Crimsonia] [Shoulder Cannon {3066482095292416}] [Event {836045448945472}: AbilityActivate {836045448945479}] ()"
,"[19:34:05.320] [@Crimsonia] [Dash'Roode {3153558262251520}:1589000694342] [Bleeding {2029831608860928}] [ApplyEffect {836045448945477}: Damage {836045448945501}] (618* internal {836045448940876}) <618>"
,"[19:34:05.513] [@Crimsonia] [Dash'Roode {3153558262251520}:1589000694342] [Plasma Burst {3234788978720768}] [ApplyEffect {836045448945477}: Damage {836045448945501}] (285 elemental {836045448940875})"
,"[19:34:05.514] [@Crimsonia] [@Crimsonia] [Charged Gauntlets {2036029246668800}] [RemoveEffect {836045448945478}: Charged Gauntlets {2036029246669056}] ()"
,"[19:34:05.514] [@Crimsonia] [Dash'Roode {3153558262251520}:1589000694342] [Rail Shot {814278554681344}] [ApplyEffect {836045448945477}: Damage {836045448945501}] (5569* energy {836045448940874}) <5569>"
,"[19:34:05.517] [@Crimsonia] [Dash'Roode {3153558262251520}:1589000694342] [Shoulder Cannon {3066494980194584}] [ApplyEffect {836045448945477}: Damage {836045448945501}] (2802* kinetic {836045448940873}) <2802>"
,"[19:34:06.040] [@Hesu] [@Crimsonia] [Kolto Missile {3163922018336768}] [ApplyEffect {836045448945477}: Heal {836045448945500}] (633*) <284>"
,"[19:34:06.343] [@Crimsonia] [Dash'Roode {3153558262251520}:1589000694342] [Bleeding {2029831608860928}] [ApplyEffect {836045448945477}: Damage {836045448945501}] (305 internal {836045448940876}) <305>"
,"[19:34:06.346] [@Crimsonia] [@Crimsonia] [Shoulder Cannon {3066482095292416}] [Event {836045448945472}: AbilityActivate {836045448945479}] ()"
,"[19:34:06.675] [@Crimsonia] [Dash'Roode {3153558262251520}:1589000694342] [Shoulder Cannon {3066494980194584}] [ApplyEffect {836045448945477}: Damage {836045448945501}] (1578 kinetic {836045448940873}) <1578>"
, "[19:34:07.077] [@Hesu] [@Crimsonia] [Kolto Missile {3163922018336768}] [ApplyEffect {836045448945477}: Heal {836045448945500}] (336) <151>"};
    string_to_id_string_map strings;
    character_list names;
    for ( auto line : lines ) {
        auto r = parse_combat_log_line(line, line + strlen(line), strings, names);
    }
}


int APIENTRY WinMain(HINSTANCE /*hInstance*/
                    ,HINSTANCE /*hPrevInstance*/
                    ,LPSTR /*lpCmdLine*/
                    ,int /*nCmdShow*/) {
                        //test_parser();
    //int arg_c;
    //auto arg_v = CommandLineToArgvW(GetCommandLine(), &arg_c);
    auto hMutex = CreateMutexW(nullptr, TRUE, L"swtor_log_analizer_unique");
    WaitForSingleObject(hMutex, INFINITE);
    try {
        app core(APP_CATION, CONFIG_PATH);

        core();
    } catch ( const std::exception& e ) {
        MessageBoxA(nullptr, e.what(), "Crash!", 0);
    } catch ( ... ) {
        MessageBoxA(nullptr, "Very bad crash", "Crash!", 0);
    }
    CloseHandle(hMutex);
}