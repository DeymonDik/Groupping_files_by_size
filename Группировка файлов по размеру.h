#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <Windows.h>

namespace fs = std::filesystem;

//��������� ��� ��������� ���������
struct GetParameter;
//��������� ������� �����
struct GetSize;
//��������� ��������� �����������
class MediaInfoWrapper;
//���������: ���� � ����� � ��������
struct ParamFile;
//���������: �������
struct Claster;
//���������: ���������� ��������������
struct Clasters;
//���������: ������ - ����
struct PathFromTo;
//����� �����������
class Grouping;

//����� ���� ������ � ������ � ��������� �������� ����������
void recursive(std::vector<ParamFile>& files, long long& summSize, const fs::path& sourceDir, GetParameter* param);
//����������� �����
void moveFile(PathFromTo&& fromTo);
//����������� �� �������
void groupFilesBySize(const PathFromTo& fromTo, const long long& sizeGroup);
//����������� �� ���������
void groupFilesByClasters(const PathFromTo& fromTo, const int& quantity);
//����������� �� ������ ��������� ����������
void groupFilesByFrequent(const PathFromTo& fromTo);
//�������� ������ �����
std::pair<int, int> deleteEmptyFolders(const fs::path& sourceDir);
//�������������� ������
void renameFiles(const std::string& sourceDir, const bool& subFolders);

//������� �������
int main();
