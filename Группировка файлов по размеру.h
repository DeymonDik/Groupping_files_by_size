#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <Windows.h>

namespace fs = std::filesystem;

//Интерфейс для получения параметра
struct GetParameter;
//Получения размера файла
struct GetSize;
//Получение параметра медиаданных
class MediaInfoWrapper;
//Структура: путь к файлу и параметр
struct ParamFile;
//Структура: кластер
struct Claster;
//Структура: управление кластеризацией
struct Clasters;
//Структура: откуда - куда
struct PathFromTo;
//Класс группировки
class Grouping;

//Обход всех файлов в папках и получение значений параметров
void recursive(std::vector<ParamFile>& files, long long& summSize, const fs::path& sourceDir, GetParameter* param);
//Перемещение файла
void moveFile(PathFromTo&& fromTo);
//Группировка по размеру
void groupFilesBySize(const PathFromTo& fromTo, const long long& sizeGroup);
//Группировка по кластерам
void groupFilesByClasters(const PathFromTo& fromTo, const int& quantity);
//Группировка по частым значениям параметров
void groupFilesByFrequent(const PathFromTo& fromTo);
//Удаление пустых папок
std::pair<int, int> deleteEmptyFolders(const fs::path& sourceDir);
//Переименование файлов
void renameFiles(const std::string& sourceDir, const bool& subFolders);

//Главная функция
int main();
