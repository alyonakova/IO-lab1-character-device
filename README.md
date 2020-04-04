# IO-lab1-character-device

[![следует шаблону standard-readme](https://img.shields.io/badge/шаблон%20readme-стандартный-brightgreen.svg?style=flat-square)](https://github.com/RichardLitt/standard-readme)

Драйвер символьного устройства.

Лабораторная работа № 1

# Установка

Чтобы собрать модуль ядра, выполните команду:
```
make all
```
Файл модуля `ch_drv.ko` появится в каталоге `out`.

Чтобы загрузить собранный модуль в ядро, используйте команду
```
make load
```
При успешной загрузке в циклическом буфере ядра отображается сообщение `«Hello!»`. Проверить это можно командой `sudo dmesg`.

Чтобы выгрузить модуль из ядра:
```
make unload
```
При выгрузку в циклическом буфере ядра отображается сообщение `«Bye!!!»`.

Также может быть полезной команда `make reload`. Эта команда выполняет последовательно выгрузку модуля из ядра (`make unload`), очистку циклического буфера ядра (`make clear-buffer`) и загрузку драйвера в ядро (`make load`).

Для разработки предлагается использовать следующую команду, выполняющую пересборку драйвера и перезагрузку модуля ядра:
```
make all reload
```

# Использование

После загрузки модуля создаётся файл символьного устройства `/dev/var1`.

Пример использования:
```
echo -n 'open filename' > /dev/var1
cat filename  # Total written bytes: 0

echo -n 1234567 > /dev/var1
cat filename  # Total written bytes: 7

echo -n 89 > /dev/var1
cat filename  # Total written bytes: 9

echo -n close > /dev/var1
echo -n ABC > /dev/var1  # Permission denied
cat filename  # Total written bytes: 9
```

# Участие

Принимаются пулл-реквесты.

Примечание: если изменяете README, следуйте шаблону
[standard-readme](https://github.com/RichardLitt/standard-readme).

# Лицензия

GNU GPL v2 или более поздняя. См. файл `LICENSE.txt`.
