# Veyon Internet Access Control

[![Windows](https://github.com/elmodos/veyon-internet-access-control/actions/workflows/windows.yml/badge.svg?branch=main)](https://github.com/elmodos/veyon-internet-access-control/actions/workflows/windows.yml)
[![Fedora](https://github.com/elmodos/veyon-internet-access-control/actions/workflows/fedora.yml/badge.svg?branch=main)](https://github.com/elmodos/veyon-internet-access-control/actions/workflows/fedora.yml)
[![Ubuntu](https://github.com/elmodos/veyon-internet-access-control/actions/workflows/ubuntu.yml/badge.svg?branch=main)](https://github.com/elmodos/veyon-internet-access-control/actions/workflows/ubuntu.yml)

[English](README.md)

Плагін для [Veyon](https://veyon.io), що додає у Veyon Master кнопку **«Заблокувати Інтернет / Розблокувати Інтернет»**. На керованих комп’ютерах із Windows він блокує вихідний інтернет-трафік через Windows Firewall, а локальна мережа й сам Veyon при цьому працюють далі.

- Керовані комп’ютери (Veyon Server): **Windows 10/11** (блокування реалізоване лише для Windows).
- Комп’ютер супервайзера (Veyon Master): Windows або **Linux** (Fedora, Debian/Ubuntu/Mint, Arch).
- На всіх комп’ютерах **Veyon 4.11.x**. Плагіни прив’язані до мінорної версії Veyon.
- Інтерфейс англійською та українською. Мова береться з налаштувань мови інтерфейсу Veyon.

## Як це працює

1. Супервайзер натискає **«Заблокувати Інтернет»** у Veyon Master. Master надсилає вибраним комп’ютерам власні налаштування плагіна, тож налаштовувати плагін треба лише на комп’ютері супервайзера.
2. На кожному керованому комп’ютері Veyon Server (працює від імені `SYSTEM`) додає до Windows Firewall правила блокування вихідного трафіку з назвами `Veyon_IAC_*`. Правила стосуються лише *публічних* адрес, без локальних мереж і дозволених вузлів із налаштувань. Користувачам не потрібні права адміністратора, і запит UAC не з’являється.
3. **«Розблокувати Інтернет»** видаляє ці правила. За замовчуванням правила також видаляються під час кожного запуску чи зупинки Veyon Server (наприклад, після перезавантаження чи виходу з системи), тож комп’ютер не залишиться заблокованим випадково.

На комп’ютері супервайзера з Windows зазвичай теж працює Veyon Server, і цей комп’ютер може бути у власному списку Veyon Master. Опція **«Ніколи не блокувати сам комп’ютер супервайзера»** (увімкнена за замовчуванням) змушує такий сервер ігнорувати команди блокування від власного Veyon Master. Вимкніть її, щоб протестувати плагін на одному комп’ютері.

### Режими блокування

| Режим | Що блокується |
| --- | --- |
| Лише вебпорти (за замовчуванням) | TCP і UDP (HTTP/3, QUIC) на вказані порти, за замовчуванням `80,443,8080,8443` |
| Весь трафік, окрім локальних мереж | Усі протоколи та порти до публічних адрес |

Додаткова опція **блокування DNS** також закриває UDP-порт 53 до зовнішніх DNS-серверів. DNS-сервер у локальній мережі (наприклад, роутер) і далі працює.

### Мережеві винятки

- **Локальні мережі** ніколи не блокуються. За замовчуванням це приватні діапазони `10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16`. Loopback і link-local адреси дозволені завжди. Якщо локальна мережа використовує публічні IP-адреси, додайте їх сюди.
- **Завжди дозволені вузли** — доменні імена або IP, напр. `wikipedia.org, intranet.example.com`. Кожен керований комп’ютер визначає адреси доменів у момент блокування. Сайти за CDN (Google, YouTube тощо) часто змінюють адреси й працюватимуть ненадійно. Якщо водночас блокується DNS, дозволені домени визначатимуться лише тоді, коли DNS-сервер є в локальній мережі.

### Обмеження

- У режимі *вебпортів* VPN і проксі на інших портах працюють. Щоб їх зупинити, використовуйте режим *всього трафіку*.
- Плагін не фільтрує вміст. Він блокує лише за адресою та портом.
- Блокування реалізоване лише для Windows. На керованих комп’ютерах із Linux чи macOS плагін ігнорує команди блокування й записує попередження в журнал. Бекенди брандмауера підключаються окремо, як додати новий — див. [CONTRIBUTING.md](CONTRIBUTING.md#adding-a-firewall-backend) (англійською).

## Встановлення

Плагін потрібно встановити **на комп’ютер супервайзера і на кожен керований комп’ютер**. Без нього Veyon Master не покаже кнопки, а керовані комп’ютери проігнорують команду.

### Windows (комп’ютер супервайзера і керовані комп’ютери)

Один і той самий файл плагіна працює у Veyon Master, Veyon Configurator і Veyon Server, тож на всіх комп’ютерах із Windows він встановлюється однаково. На комп’ютері супервайзера після встановлення перезапустіть Veyon Master.

**Інсталятор (рекомендовано):** завантажте з релізів `veyon-internet-access-control-<версія>-win64-setup.exe` і запустіть. Він знаходить встановлений Veyon, перевіряє, що це Veyon 4.11, кладе плагін у теку `plugins` і перезапускає службу Veyon. Плагін з’являється в *Програмах і засобах*, звідки його можна видалити; під час видалення прибираються й усі правила брандмауера плагіна. Для віддаленого чи масового розгортання виконайте від імені адміністратора:

```bat
veyon-internet-access-control-0.0.1-win64-setup.exe /S
"C:\Program Files\Veyon Internet Access Control\uninstall.exe" /S
```

Тихе встановлення переривається, якщо встановлено Veyon не версії 4.11.

**Скрипти:** як альтернатива — завантажте `veyon-internet-access-control-win64.zip`, розпакуйте й двічі клацніть `install.cmd` або виконайте від імені адміністратора `powershell -ExecutionPolicy Bypass -File install.ps1`. `uninstall.cmd` / `uninstall.ps1` видаляють плагін і його правила брандмауера.

> Встановлення чи оновлення Veyon видаляє всі плагіни з його теки `plugins`. Після кожного оновлення Veyon встановлюйте плагін знову.

### Linux (комп’ютер супервайзера)

| Дистрибутив | Пакунок |
| --- | --- |
| Fedora | `sudo dnf install ./veyon-plugin-internetaccesscontrol-*.rpm` |
| Debian / Ubuntu / Mint | `sudo apt install ./veyon-plugin-internetaccesscontrol_*.deb` |
| Arch | `cd packaging/arch && makepkg -si` (потрібен `veyon-bin` з AUR) |

Потім перезапустіть Veyon Master.

На будь-якій платформі плагін налаштовується на комп’ютері супервайзера у **Veyon Configurator → Керування доступом до Інтернету**.

## Діагностика

На керованому комп’ютері виконайте від імені адміністратора:

```powershell
powershell -ExecutionPolicy Bypass -File diagnose.ps1               # стан, активні правила, зв’язок
powershell -ExecutionPolicy Bypass -File diagnose.ps1 -RemoveRules  # аварійне розблокування
```

Докладний журнал: у Veyon Configurator → Загальні встановіть рівень журналу *Налагодження*, потім шукайте повідомлення `InternetAccessControl` у журналі Veyon Server.

## Збирання

Вимоги:

- CMake ≥ 3.21
- компілятор C++17
- Qt ≥ 6.4 (Core, Gui, Widgets, Network, LinguistTools)
- встановлений Veyon, що містить `libveyon-core.so` / `veyon-core.dll`

Заголовки Veyon завантажуються автоматично для `VEYON_VERSION` (за замовчуванням `4.11.4`), якщо не задано `VEYON_SOURCE_DIR`.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DIAC_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
sudo cmake --install build      # встановлює поруч із libveyon-core.so
```

Опції CMake і команди для пакунків описано в [README.md](README.md#building).

**Важливо для Windows:** DLL потрібно збирати з **тією самою версією Qt, яку постачає інсталятор Veyon** (`WINDOWS_QT_VERSION` у `.github/workflows/windows.yml`), інакше Veyon падає. Тому DLL збирає GitHub Actions. Скрипт NSIS-інсталятора — `packaging/windows/installer.nsi`; `makensis` працює й на Linux (`mingw32-nsis` на Fedora, `nsis` на Debian/Ubuntu). Тег `v*` запускає `.github/workflows/release.yml`, який публікує інсталятор і zip для Windows, RPM і DEB як GitHub-реліз.

### Переклади

Переклади лежать у `translations/*.ts` і вбудовуються в плагін. Щоб оновити їх після зміни рядків у коді:

```sh
cmake --build build --target update_translations
```

Потім перекладіть нові рядки в Qt Linguist. Щоб додати мову, додайте `.ts`-файл у `qt_add_translations()` у `CMakeLists.txt`.

## Ліцензія

[MIT](LICENSE). Базується на аддоні Internet Access Control із [Veyon Community Addons](https://github.com/YellowZaki/Veyon-Community-Addons).
