# rmc_parser
nmea rmc message parser

Как добавить зависимости
Создать на github новый проект project1
склонировать его в локальную папку.
в локальной папке создать директорию external
добавить зависимость depend1:
git submodule add https://github.com/wiluite/depend1 external/depend1

В git-gui зафиксировать изменения (staged)
commit
push на github.

зависимость depend1 внутри проекта project1 должна быть видна как depend1 @ 1234567

при повторном клонировании с github проекта project1 в пустую локальную папку
исходные файлы зависимости появляться не должны.

