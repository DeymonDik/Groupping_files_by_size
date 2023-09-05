#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

long long& getSize(const fs::path& file) {

    std::ifstream inputFile(file, std::ios::binary);
    long long fileSize = NULL;

    if (!inputFile)
    {
        std::cout << __format_string("Failed to open file : %s", file.string());
        return fileSize;
    }

    // Получаем размер файла
    inputFile.seekg(0, std::ios::end);
    fileSize = inputFile.tellg();
    inputFile.close();

    return fileSize;
}

void recursive(std::map<fs::path, long long>& files, long long& summSize, const std::string& sourceDir) {
    for (const auto& entry : fs::directory_iterator(sourceDir))
    {
        if (fs::is_regular_file(entry.path()))
        {
            long long size = getSize(entry.path());
            summSize += size;
            files[entry.path()] = size;
        }
        else if (fs::is_directory(entry.path())) {
            recursive(files, summSize, entry.path().string());
        }
    }
}

void groupFilesBySize(const std::string& sourceDir, const std::string& destinationDir, const long long& sizeGroup)
{
    // Создаем папку назначения, если она не существует
    fs::create_directory(destinationDir);

    // Получаем список всех файлов в исходной и вложенных папках
    std::map<fs::path, long long> files;
    long long summSize = 0;
    recursive(files, summSize, sourceDir);
    if (!summSize)
    {
        std::cout << "Файлов не найдено!" << std::endl;
        return;
    }
    // Создаём папки
    std::map<fs::path, long long> folders;
    int countFolders = (int)(summSize / sizeGroup);
    for (size_t i = 1; i <= (countFolders ? countFolders : 1); i++)
    {
        std::string sizeFolder = destinationDir + "\\" + std::to_string(i);
        fs::create_directory(sizeFolder);
        folders[sizeFolder] = 0;
    }

    // Сортировка файлов по убыванию размера
    std::vector<std::pair<fs::path, long long>> sortedFiles(files.begin(), files.end());
    std::sort(sortedFiles.begin(), sortedFiles.end(), [](const auto& o1, const auto& o2) {return o1.second > o2.second; });

    // Группировка файлов по папкам
    for (const auto& file : sortedFiles)
    {
        // Нахождение минимального значения размера папки
        auto minFolder = std::min_element(folders.begin(), folders.end(), [](const auto& o1, const auto& o2) {return o1.second < o2.second; });
        (*minFolder).second += file.second;

        // Путь назначения для перемещения файла
        fs::path destinationFile = (*minFolder).first.string() + "\\" + file.first.filename().string();

        // Введём дополнительные переменные
        std::string stem;
        std::string extension;
        int erIndex = 0;
        int numberInt = 0;
        std::cout << std::setiosflags(std::ios::left);

        // Попробуем за 100 раз переместить файл
        do
        {
            try {

                // Перемещаем файл в соответствующую папку переименованием
                fs::rename(file.first, destinationFile);

                // Выводит на экран успешный результат выполнения
                std::cout << std::setw(27) << "Успех: " << std::setw(48) << file.first.string() << " -> " << destinationFile.string() << std::endl;
                break;
            }
            catch (const std::exception& e) {

                // Сохраним как копию
                if (!numberInt)
                {
                    // Разделим имя на стем и расширение
                    stem = destinationFile.stem().string();
                    extension = destinationFile.extension().string();

                    // Добавляем к имени номер копии (1)
                    if (stem.back() == ')')
                    {
                        std::string number = "";
                        while (stem.erase(stem.length() - 1).back() != '(')
                        {
                            number.insert(0, 1, stem.back());
                        }
                        numberInt = std::stoi(number);
                    }
                    else
                    {
                        stem += "(";
                    }
                }
                destinationFile.replace_filename(stem + std::to_string(++numberInt) + ")" + extension);
            }
        } while (++erIndex < 100);

        // Выводит на экран неуспешный результат выполнения
        if (erIndex >= 100) {
            std::cout << std::setw(27) << "Ошибка: " << std::setw(48) << file.first.string() << " -> " << destinationFile.string() << std::endl;
        }
    }
}

std::pair<int, int>& deleteEmptyFolders(const std::string& sourceDir) {
    std::pair<int, int> count{ 0, 0 };
    for (const auto& entry : fs::directory_iterator(sourceDir))
    {
        if (fs::is_regular_file(entry.path()))
        {
            ++count.second;
        }
        else if (fs::is_directory(entry.path())) {
            std::pair<int, int> nextCount = deleteEmptyFolders(entry.path().string());
            if (!nextCount.second) {
                if (fs::remove(entry.path())) {
                    ++count.first;
                }
                else
                {
                    std::cout << "Пустая папка: " << entry.path() << " не удалена!" << std::endl;
                }
            }
            else {
                count.second += nextCount.second;
            }
            count.first += nextCount.first;
        }
    }
    return count;
}

void renameFiles(const std::string& sourceDir, const bool& subFolders = false) {
    int i = 0;
    for (const auto& entry : fs::directory_iterator(sourceDir))
    {
        if (fs::is_regular_file(entry.path()))
        {
            // Путь назначения для перемещения файла
            fs::path destinationFile = entry.path().parent_path().string() + "\\" + std::to_string(++i) + entry.path().extension().string();

            // Введём дополнительные переменные
            std::string stem;
            std::string extension;
            int erIndex = 0;
            int numberInt = 0;
            std::cout << std::setiosflags(std::ios::left);

            // Попробуем за 100 раз переместить файл
            do
            {
                try {

                    // Перемещаем файл в соответствующую папку переименованием
                    fs::rename(entry.path(), destinationFile);

                    // Выводит на экран успешный результат выполнения
                    std::cout << std::setw(27) << "Успех: " << std::setw(48) << entry.path().string() << " -> " << destinationFile.string() << std::endl;
                    return;
                }
                catch (const std::exception& e) {

                    // Сохраним как копию
                    if (!numberInt)
                    {
                        // Разделим имя на стем и расширение
                        stem = destinationFile.stem().string();
                        extension = destinationFile.extension().string();

                        // Добавляем к имени номер копии (1)
                        if (stem.back() == ')')
                        {
                            std::string number = "";
                            while (stem.erase(stem.length() - 1).back() != '(')
                            {
                                number.insert(0, 1, stem.back());
                            }
                            numberInt = std::stoi(number);
                        }
                        else
                        {
                            stem += "(";
                        }
                    }
                    destinationFile.replace_filename(stem + std::to_string(++numberInt) + ")" + extension);
                }
            } while (++erIndex < 100);

            // Выводит на экран неуспешный результат выполнения
            std::cout << std::setw(27) << "Ошибка: " << std::setw(48) << entry.path().string() << " -> " << destinationFile.string() << std::endl;
        }
        else if (subFolders && fs::is_directory(entry.path())) {
            renameFiles(entry.path().string(), subFolders);
        }
    }
}

int main()
{
    setlocale(LC_ALL, "");

    std::string sourceDir = "Res";
    std::string destinationDir = "Dest";
    long long sizeGroup = 0;
    int choise = 0;

START:
    std::cout << "Введите номер Б = 0, КБ = 1, МБ = 2, ГБ = 3: ";
    std::cin >> choise;

    std::cout << "Введите размер группы файлов: ";
    std::cin >> sizeGroup;

    switch (choise)
    {
    case 3:
        sizeGroup *= 1000;
    case 2:
        sizeGroup *= 1000;
    case 1:
        sizeGroup *= 1000;
    case 0:
        break;
    default:
        system("cls");
        goto START;
    }

    groupFilesBySize(sourceDir, destinationDir, sizeGroup);

    auto pair = deleteEmptyFolders(sourceDir);

    std::cout << "Удалено папок: " << pair.first << std::endl;
    std::cout << "Файлов обнаружено при удалении: " << pair.second << std::endl;

    //renameFiles(destinationDir, true);

    system("pause");

    return 0;
}