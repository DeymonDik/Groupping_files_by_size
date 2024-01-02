#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <Windows.h>

namespace fs = std::filesystem;

struct GetParameter {
	virtual long long getParameter(const fs::path& entryPath) const = 0;
};

struct GetSize : public GetParameter {
	virtual long long getParameter(const fs::path& file) const {

		std::ifstream inputFile(file, std::ios::binary);
		long long fileSize = NULL;

		if (!inputFile)
		{
			std::cout << "Failed to open file :" + file.string();
			return fileSize;
		}

		// Получаем размер файла
		inputFile.seekg(0, std::ios::end);
		fileSize = inputFile.tellg();
		inputFile.close();

		return fileSize;
	}
};

class MediaInfoWrapper : public GetParameter {

	HMODULE hLib = NULL;
	HANDLE handle = NULL;

	typedef enum MediaInfo_stream_t {
		MediaInfo_Stream_General,
		MediaInfo_Stream_Video,
		MediaInfo_Stream_Audio,
		MediaInfo_Stream_Text,
		MediaInfo_Stream_Other,
		MediaInfo_Stream_Image,
		MediaInfo_Stream_Menu,
		MediaInfo_Stream_Max
	} MediaInfo_stream_C;

	typedef enum MediaInfo_info_t {
		MediaInfo_Info_Name,
		MediaInfo_Info_Text,
		MediaInfo_Info_Measure,
		MediaInfo_Info_Options,
		MediaInfo_Info_Name_Text,
		MediaInfo_Info_Measure_Text,
		MediaInfo_Info_Info,
		MediaInfo_Info_HowTo,
		MediaInfo_Info_Max
	} MediaInfo_info_C;

	typedef HANDLE(__stdcall* MEDIAINFO_New)();
	typedef size_t(__stdcall* MEDIAINFO_Open)(HANDLE, const wchar_t*);
	typedef const wchar_t* (__stdcall* MEDIAINFO_Get)(HANDLE, MediaInfo_stream_C StreamKind, size_t StreamNumber, const wchar_t* Parameter, MediaInfo_info_C KindOfInfo, MediaInfo_info_C KindOfSearch);
	typedef void(__stdcall* MEDIAINFO_Close)(HANDLE);
	//typedef const wchar_t* (__stdcall* MEDIAINFO_Inform)(HANDLE, size_t Reserved);
	//typedef const wchar_t* (__stdcall* MEDIAINFO_GetI)(HANDLE, MediaInfo_stream_C StreamKind, size_t StreamNumber, size_t Parameter, MediaInfo_info_C KindOfInfo);

	MEDIAINFO_New newFunc = NULL;
	MEDIAINFO_Open openFunc = NULL;
	MEDIAINFO_Get getFunc = NULL;
	MEDIAINFO_Close closeFunc = NULL;
	//MEDIAINFO_GetI getIFunc = NULL;
	//MEDIAINFO_Inform informFunc = NULL;
public:
	MediaInfoWrapper() {
		hLib = LoadLibrary(L"MediaInfo.dll");
		if (!hLib)
		{
			std::cout << "Библиотека MediaInfo.dll не найдена!" << std::endl;
			return;
		}
		newFunc = (MEDIAINFO_New)GetProcAddress(hLib, "MediaInfo_New");
		openFunc = (MEDIAINFO_Open)GetProcAddress(hLib, "MediaInfo_Open");
		getFunc = (MEDIAINFO_Get)GetProcAddress(hLib, "MediaInfo_Get");
		closeFunc = (MEDIAINFO_Close)GetProcAddress(hLib, "MediaInfo_Close");
		//getIFunc = (MEDIAINFO_GetI)GetProcAddress(hLib, "MediaInfo_GetI");
		//informFunc = (MEDIAINFO_Inform)GetProcAddress(hLib, "MediaInfo_Inform");

		handle = newFunc();
	}

	~MediaInfoWrapper() {
		FreeLibrary(hLib);
	}

	//const wchar_t* convert(const char* str) const {
	//	size_t len = std::strlen(str) + 1;
	//	size_t convertedChars = 0;
	//	wchar_t* wstr = new wchar_t[len];
	//	mbstowcs_s(&convertedChars, wstr, len, str, len);
	//	return wstr;
	//}

	virtual long long getParameter(const fs::path& entryPath) const override {
		return getQuality(entryPath.c_str());
	}

	const long long getQuality(const wchar_t* wFilePath) const {
		openFunc(handle, wFilePath);
		const wchar_t* text = getFunc(handle, MediaInfo_Stream_General, 0, L"OverallBitRate", MediaInfo_Info_Text, MediaInfo_Info_Name);
		closeFunc(handle);
		return std::wcstoll(text, nullptr, 10);
	}

};

struct ParamFile {
	fs::path filePath;
	long long fileParam;
};

struct Claster {
	bool isMerged{};
	std::vector<int> mergedObjects{};
	long long difference{};

	void merge(Claster& resource) {
		for (auto res : resource.mergedObjects) {
			mergedObjects.push_back(res);
		}
		resource.isMerged = true;
	}
};

struct Clasters {

	size_t size{};
	std::vector<Claster> clasters{};

	Clasters(std::vector<ParamFile>& sortedFiles) {

		size = sortedFiles.size();

		for (size_t i = 0; i < size - 1; i++)
		{
			Claster pair;
			pair.mergedObjects.push_back((int)i);
			pair.difference = sortedFiles.at(i + 1).fileParam - sortedFiles.at(i).fileParam;
			clasters.push_back(pair);
		}
		std::sort(clasters.begin(), clasters.end(), [](const auto& o1, const auto& o2) {return o1.difference < o2.difference; });

		Claster last;
		last.mergedObjects.push_back((int)size - 1);
		last.difference = -1;
		clasters.push_back(last);

	}

	Claster& at(const int& index) {
		return *std::find_if(clasters.begin(), clasters.end(), [&index](const auto& o1) {
			return !o1.isMerged && o1.mergedObjects.end() != std::find_if(o1.mergedObjects.begin(), o1.mergedObjects.end(), [&index](const auto& l1) {return l1 == index; });
			}
		);
	}

	void clastering(const int& quantity) {
		for (size_t i = 0; size > quantity; size--, i++)
		{
			Claster& claster = clasters.at(i);
			int j = claster.mergedObjects[claster.mergedObjects.size() - 1];
			at(j).merge(at(j + 1));
		}
		deleteMerged();
	}

	void deleteMerged() {
		auto it = clasters.begin();
		int i{};
		while (it != clasters.end()) {
			if ((*it).isMerged)
			{
				it = clasters.erase(it);
			}
			else {
				it++;
			}
		}
	}

};

struct PathFromTo {
	fs::path from;
	fs::path to;
};

class Grouping {

};

void recursive(std::vector<ParamFile>& files, long long& summSize, const fs::path& sourceDir, GetParameter* param) {
	for (const auto& entry : fs::directory_iterator(sourceDir))
	{
		if (fs::is_regular_file(entry.path()))
		{
			long long size = param->getParameter(entry.path());
			summSize += size;
			files.push_back({entry.path(), size});
		}
		else if (fs::is_directory(entry.path())) {
			recursive(files, summSize, entry.path(), param);
		}
	}
}

void moveFile(PathFromTo&& fromTo) {

	// Введём дополнительные переменные
	std::wstring stem;
	std::wstring extension;
	int erIndex = 0;
	int numberInt = 0;
	std::wcout << std::setiosflags(std::ios::left);

	// Попробуем за 100 раз переместить файл
	do
	{
		try {

			// Перемещаем файл в соответствующую папку переименованием
			fs::rename(fromTo.from, fromTo.to);

			// Выводит на экран успешный результат выполнения
			std::wcout << std::setw(27) << L"Успех: " << std::setw(48) << fromTo.from << L" -> " << fromTo.to << std::endl;
			break;
		}
		catch (const std::exception&) {

			// Сохраним как копию
			if (!numberInt)
			{
				// Разделим имя на корень и расширение
				stem = fromTo.to.stem().wstring();
				extension = fromTo.to.extension().wstring();

				// Добавляем к имени номер копии (1)
				if (stem.back() == L')')
				{
					std::wstring number = L"";
					while (stem.erase(stem.length() - 1).back() != L'(')
					{
						number.insert(0, 1, stem.back());
					}
					numberInt = std::stoi(number);
				}
				else
				{
					stem += L"(";
				}
			}
			fromTo.to.replace_filename(stem + std::to_wstring(++numberInt) + L")" + extension);
		}
	} while (++erIndex < 100);

	// Выводит на экран неуспешный результат выполнения
	if (erIndex > 99) {
		std::wcout << std::setw(27) << L"Ошибка: " << std::setw(48) << fromTo.from << L" -> " << fromTo.to << std::endl;
	}
}

void groupFilesBySize(const PathFromTo& fromTo, const long long& sizeGroup)
{
	// Создаем папку назначения, если она не существует
	fs::create_directory(fromTo.to);

	// Получаем список всех файлов в исходной и вложенных папках
	std::vector<ParamFile> files;
	long long summSize = 0;
	GetSize* getSize = new GetSize();
	recursive(files, summSize, fromTo.from, getSize);
	delete getSize;
	if (!summSize)
	{
		std::cout << "Файлов не найдено!" << std::endl;
		return;
	}
	// Создаём папки
	std::vector<ParamFile> folders;
	int countFolders = (int)(summSize / sizeGroup);
	for (size_t i = 1; i <= (countFolders ? countFolders : 1); i++)
	{
		fs::path sizeFolder = fromTo.to.wstring() + L"\\" + std::to_wstring(i);
		fs::create_directory(sizeFolder);
		folders.push_back({sizeFolder, 0});
	}

	// Сортировка файлов по убыванию размера
	std::sort(files.begin(), files.end(), [](const auto& o1, const auto& o2) {return o1.fileParam > o2.fileParam; });

	// Группировка файлов по папкам
	for (const auto& file : files)
	{
		// Нахождение минимального значения размера папки
		auto minFolder = std::min_element(folders.begin(), folders.end(), [](const auto& o1, const auto& o2) {return o1.fileParam < o2.fileParam; });
		(*minFolder).fileParam += file.fileParam;

		// Путь назначения для перемещения файла
		fs::path destinationFile = (*minFolder).filePath.string() + "\\" + file.filePath.filename().string();

		// Перемещаем файл
		moveFile({ file.filePath, destinationFile });
	}
}

void groupFilesByClasters(const PathFromTo& fromTo, const int& quantity) {
	// Создаем папку назначения, если она не существует
	fs::create_directory(fromTo.to);

	// Получаем список всех файлов в исходной и вложенных папках
	std::vector<ParamFile> files;
	long long summSize = 0;
	GetParameter* get = new MediaInfoWrapper();
	recursive(files, summSize, fromTo.from, get);
	delete get;
	if (!summSize)
	{
		std::cout << "Файлов не найдено!" << std::endl;
		return;
	}

	// Сортировка файлов по возрастанию размера
	std::sort(files.begin(), files.end(), [](const auto& o1, const auto& o2) {return o1.fileParam < o2.fileParam; });

	// Кластеризация
	Clasters clastering(files);
	clastering.clastering(quantity);

	// Группировка файлов путём кластеризации
	for (size_t i = 0; i < clastering.size; i++)
	{
		// Получение данных из кластеров
		Claster& claster = clastering.clasters.at(i);
		int begin = claster.mergedObjects[0];
		int end = claster.mergedObjects[claster.mergedObjects.size() - 1];

		// Создание папки
		std::wstring sBegin = std::to_wstring(files.at(begin).fileParam / 1024 / 1024);
		std::wstring sEnd = std::to_wstring(files.at(end).fileParam / 1024 / 1024);
		std::wstring folder = fromTo.to.wstring() + L"\\" + sBegin + L"-" + sEnd;
		fs::create_directory(folder);

		// Перемещение файлов в созданную папку
		for (size_t j = begin; j <= end; j++)
		{
			// Перемещение файла
			fs::path destinationFile = folder + L"\\" + files[j].filePath.filename().wstring();
			moveFile({ files[j].filePath, destinationFile });
		}
	}
}

void groupFilesByFrequent(const PathFromTo& fromTo) {
	// Создаем папку назначения, если она не существует
	fs::create_directory(fromTo.to);

	// Получаем список всех файлов в исходной и вложенных папках
	std::vector<ParamFile> files;
	long long summSize = 0;
	GetSize* getSize = new GetSize();
	recursive(files, summSize, fromTo.from, getSize);
	delete getSize;
	if (!summSize)
	{
		std::cout << "Файлов не найдено!" << std::endl;
		return;
	}

	// Сортировка файлов по возрастанию размера
	std::sort(files.begin(), files.end(), [](const auto& o1, const auto& o2) {return o1.fileParam < o2.fileParam; });

	// Группировка файлов по частым размерам
	long long back = files.back().fileParam * 10;
	long long digit = 1;
	int level = 1;
	do
	{
		for (int i = 0; i < (int)files.size() - 1;)
		{
			int j = i + 1;
			if (files[i].fileParam / digit == files[j].fileParam / digit)
			{
				std::wstring folder = fromTo.to.wstring() + L"\\" + std::to_wstring(level);
				fs::create_directory(folder);

				do {
					fs::path destinationFile = folder + L"\\" + files[j].filePath.filename().wstring();
					moveFile({ files[j].filePath, destinationFile });
					files.erase(files.begin() + j);
				} while (j < files.size() && files[i].fileParam / digit == files[j].fileParam / digit);

				fs::path destinationFile = folder + L"\\" + files[i].filePath.filename().wstring();
				moveFile({ files[i].filePath, destinationFile });
				files.erase(files.begin() + i);
			}
			else
			{
				++i;
			}
		}
		digit *= 10;
		++level;
	} while (back / digit);

}

std::pair<int, int> deleteEmptyFolders(const fs::path& sourceDir) {
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
				catch (const std::exception&) {

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

	//int choise{}, sizeGroup{};
	PathFromTo fromTo{L"Res", L"Dest"};

//START:
//	std::cout << "Введите номер Б = 0, КБ = 1, МБ = 2, ГБ = 3: ";
//	std::cin >> choise;
//
//	std::cout << "Введите размер группы файлов: ";
//	std::cin >> sizeGroup;
//
//	switch (choise)
//	{
//	case 3:
//		sizeGroup *= 1000;
//	case 2:
//		sizeGroup *= 1000;
//	case 1:
//		sizeGroup *= 1000;
//	case 0:
//		break;
//	default:
//		system("cls");
//		goto START;
//	}

	//groupFilesByFrequent(sourceDir, destinationDir);

	groupFilesByClasters(fromTo, 4);

	auto pair = deleteEmptyFolders(fromTo.from);

	std::cout << "Удалено папок: " << pair.first << std::endl;
	std::cout << "Файлов обнаружено при удалении: " << pair.second << std::endl;

	//renameFiles(destinationDir, true);

	system("pause");

	return 0;
}