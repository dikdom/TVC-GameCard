# TVC-GameCard
Egy IO bővítőkártya a Videoton TV Computerhez, melyen 2 extra joy port és egy SN76489 hangchip található. Ez a kártya a TVC játékos lehetőségeit
bővíti a jobb zenei lehetőségeivel és a 4 játékos üzemmóddal.
An IO expander card for Videoton TV Computer having +2 joy port and an SN76489 sound chip.

Ez a repo a méltán nagysikerű Videoton TV Computerhez készített TVC-GameCard IO kártya leírását és újragyártásához szükséges file-okat tartalmazza.
A kártya 2 extra joy portot, egy SN76489 hangchipet (+3 hangcsatorna +1 zajcsatorna) és 4 extra játékot tartalmaz. Bekapcsoláskor a felhasználó
választhat a játékok közül, de akár tovább is léphet a sima, sztenderd BASIC bekapcsolási képernyőre (...khm... felé, mert ha van több IO kártya
is bedugva a gépbe, akkor simán lehet, hogy a következő kártya fogja megfogni a boot folyamatát..)

Könyvtárstruktúra:
- `doc` dokumentumok a kártyáról
- `src`
  - `menu`
    a boot menü forráskódja, ami az ASM80.com oldalon található IDE-vel készült
  - `gal`
    a GAL20V8B chip kódjainak a forrásai, amit a WinCUPL programmal lehet lefordítani
  - `hw` kapcsolási rajz és board layout EagleCAD formátumban
  - 'tvc4snake' a TVC4Snake játék forrása
- `bin`
  minden, ami egy kártya elkészítéséhez szükséges: a gerber file csomagok (TH és SMD konfiguráció, az SMD -hez a gyártási file-ok is a JLC felé), 
  a GAL-ok lefordított .jed file-jai és a EPROM-ba írható bináris, ami a lefordított  boot menüt és a játékokat is tartalmazza

MIT License: minden használható/módosítható, de forrást tessék feltüntetni
