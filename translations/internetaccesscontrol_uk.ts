<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="uk_UA">
<context>
    <name>InternetAccessControlConfigurationPage</name>
    <message>
        <source>Internet Access Control</source>
        <translation>Керування доступом до Інтернету</translation>
    </message>
    <message>
        <source>These settings are configured on the supervisor computer and sent to the managed computers each time the internet gets blocked.</source>
        <translation>Ці параметри налаштовуються на комп’ютері супервайзера й надсилаються на керовані комп’ютери під час кожного блокування Інтернету.</translation>
    </message>
    <message>
        <source>General settings</source>
        <translation>Загальні параметри</translation>
    </message>
    <message>
        <source>Blocking mode:</source>
        <translation>Режим блокування:</translation>
    </message>
    <message>
        <source>Block web ports only (HTTP, HTTPS, QUIC)</source>
        <translation>Блокувати лише вебпорти (HTTP, HTTPS, QUIC)</translation>
    </message>
    <message>
        <source>Block all traffic except local networks</source>
        <translation>Блокувати весь трафік, окрім локальних мереж</translation>
    </message>
    <message>
        <source>Blocked ports (TCP/UDP):</source>
        <translation>Заблоковані порти (TCP/UDP):</translation>
    </message>
    <message>
        <source>Comma-separated ports or port ranges, e.g. 80,443,8000-8999. Only used when blocking web ports.</source>
        <translation>Порти або діапазони портів через кому, напр. 80,443,8000-8999. Використовується лише в режимі блокування вебпортів.</translation>
    </message>
    <message>
        <source>Block DNS queries to external servers (UDP port 53)</source>
        <translation>Блокувати DNS-запити до зовнішніх серверів (UDP-порт 53)</translation>
    </message>
    <message>
        <source>Read from the configuration of each managed computer.</source>
        <translation>Береться з конфігурації кожного керованого комп’ютера.</translation>
    </message>
    <message>
        <source>Restore internet access when the Veyon Server starts or stops (e.g. on reboot or logoff)</source>
        <translation>Відновлювати доступ до Інтернету під час запуску чи зупинки Veyon Server (напр. після перезавантаження чи виходу з системи)</translation>
    </message>
    <message>
        <source>Ignores the block command on the computer running Veyon Master, e.g. when it is part of its own computer list. Disable this to test the plugin on a single computer.</source>
        <translation>Ігнорує команду блокування на комп’ютері, де працює Veyon Master, напр. коли він є у власному списку комп’ютерів. Вимкніть, щоб протестувати плагін на одному комп’ютері.</translation>
    </message>
    <message>
        <source>Never block the supervisor computer itself</source>
        <translation>Ніколи не блокувати сам комп’ютер супервайзера</translation>
    </message>
    <message>
        <source>Network exceptions</source>
        <translation>Мережеві винятки</translation>
    </message>
    <message>
        <source>Local networks (never blocked):</source>
        <translation>Локальні мережі (ніколи не блокуються):</translation>
    </message>
    <message>
        <source>Comma-separated IP addresses or subnets in CIDR notation. Loopback and link-local addresses are always allowed.</source>
        <translation>IP-адреси або підмережі в нотації CIDR через кому. Адреси loopback і link-local дозволені завжди.</translation>
    </message>
    <message>
        <source>Always allowed hosts (domains or IPs):</source>
        <translation>Завжди дозволені вузли (домени або IP):</translation>
    </message>
    <message>
        <source>Domain names are resolved on each managed computer when the internet gets blocked. Websites served via CDNs may use many changing addresses and therefore not work reliably.</source>
        <translation>Доменні імена визначаються на кожному керованому комп’ютері в момент блокування Інтернету. Сайти, що працюють через CDN, можуть мати багато змінних адрес і тому працювати ненадійно.</translation>
    </message>
    <message>
        <source>e.g. wikipedia.org, intranet.example.com, 203.0.113.10</source>
        <translation>напр. wikipedia.org, intranet.example.com, 203.0.113.10</translation>
    </message>
</context>
<context>
    <name>InternetAccessControlFeaturePlugin</name>
    <message>
        <source>Block or restore internet access on managed computers</source>
        <translation>Блокування та відновлення доступу до Інтернету на керованих комп’ютерах</translation>
    </message>
    <message>
        <source>Block Internet</source>
        <translation>Заблокувати Інтернет</translation>
    </message>
    <message>
        <source>Unblock Internet</source>
        <translation>Розблокувати Інтернет</translation>
    </message>
    <message>
        <source>Block or allow internet access on selected computers while keeping local network access active.</source>
        <translation>Блокує або дозволяє доступ до Інтернету на вибраних комп’ютерах, зберігаючи доступ до локальної мережі.</translation>
    </message>
</context>
</TS>
