#include <Windows.h>
#include <iostream>
#include <mmsystem.h>
#include <math.h>

#pragma comment (lib, "Winmm.lib")

unsigned int Done = 0;

VOID CALLBACK ReadDone(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped) 
{
	Done++;
};

VOID CALLBACK WriteDone(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	Done++;
};

void main(void)
{
	setlocale(LC_ALL, "Russian");

	unsigned int BlockSize = 0;
	std::cout << "������� ������ �����, ������� ������� �������� (� ������): ";
	std::cin >> BlockSize;

	unsigned int OpNumber = 0;
	std::cout << "������� ����� ��������������� �������� �����/������: ";
	std::cin >> OpNumber;

	char *cPath1 = new char[256];
	wchar_t* mPath1 = new wchar_t[256];
	char *cPath2 = new char[256];
	wchar_t* mPath2 = new wchar_t[256];

	std::cout << "������� ��� �����, ������� ����� �����������:" << std::endl;
	std::cin >> cPath1;
	mbstowcs_s(NULL, mPath1, 256, cPath1, _TRUNCATE);

	std::cout << "������� ��� �����, � ������� ����� �����������:" << std::endl;
	std::cin >> cPath2;
	mbstowcs_s(NULL, mPath2, 256, cPath2, _TRUNCATE);

	HANDLE hdl1 = CreateFile(mPath1, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL);
	
	if (hdl1 == INVALID_HANDLE_VALUE)
	{
		std::cout << "������ ��� ���������� ����� " << cPath1 << "." << std::endl;
		std::cin >> BlockSize;
		return;
	};

	HANDLE hdl2 = CreateFile(mPath2, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL);
	
	if (hdl2 == INVALID_HANDLE_VALUE)
	{
		std::cout << "������ ��� ������ ����� " << cPath2 << "." << std::endl;
		std::cin >> BlockSize;
		return;
	};
	
	BY_HANDLE_FILE_INFORMATION FileInfo;

	if (!GetFileInformationByHandle(hdl1, &FileInfo))
	{
		std::cout << "������ ��� ��������� ��������� ����� \"" << cPath1 << "\"." << std::endl;
		std::cin >> BlockSize;
		return;
	};

	unsigned int BlocksCount = ceil((float)(FileInfo.nFileSizeLow + (FileInfo.nFileSizeHigh << 32)) / BlockSize);
	std::cout << "������ ����� " << cPath1 << ": " << FileInfo.nFileSizeLow + (FileInfo.nFileSizeHigh << 32) << " ����." << std::endl;
	std::cout << "����� ������: " << BlocksCount << std::endl;

	char **MassBuffer = new char*[16];
	for (unsigned int i = 0; i < 16; i++)
		MassBuffer[i] = new char[BlockSize];

	unsigned int BaseOffset = 0;

	OVERLAPPED rdFile[16];
	OVERLAPPED wrFile[16];

	unsigned int StartTime = timeGetTime();

	switch (OpNumber)
	{
	case 1:
		rdFile[0].Offset = 0;
		rdFile[0].OffsetHigh = 0;
		rdFile[0].hEvent = 0;
		rdFile[0].Pointer = 0;

		wrFile[0].Offset = 0;
		wrFile[0].OffsetHigh = 0;
		wrFile[0].hEvent = 0;
		wrFile[0].Pointer = 0;

		StartTime = timeGetTime();
		for (unsigned int i = 0; i < BlocksCount; i++)
		{
			if (!ReadFileEx(hdl1, MassBuffer[0], BlockSize, &rdFile[0], &ReadDone))
			{
				std::cout << "������ ��� ���������� ����� " << cPath1 << "." << std::endl;
				std::cin >> BlockSize;
				return;
			};

			//rdFile[0].OffsetHigh += (rdFile[0].Offset + BlockSize) >> 32;
			rdFile[0].Offset += BlockSize;

			SleepEx(INFINITE, TRUE);

			if (!WriteFileEx(hdl2, MassBuffer[0], BlockSize, &wrFile[0], &WriteDone))
			{
				std::cout << "������ ��� ������ ����� " << cPath2 << "." << std::endl;
				std::cin >> BlockSize;
				return;
			};

			wrFile[0].Offset = wrFile[0].Offset + BlockSize;

			SleepEx(INFINITE, TRUE);
		};
		break;
	case 2:
	case 4:
	case 8:
	case 12:
	case 16:
		BaseOffset = 0;

		for (unsigned int i = 0; i < OpNumber; i++)
		{
			rdFile[i].Offset = 0;
			rdFile[i].OffsetHigh = 0;
			rdFile[i].hEvent = 0;
			rdFile[i].Pointer = 0;

			wrFile[i].Offset = 0;
			wrFile[i].OffsetHigh = 0;
			wrFile[i].hEvent = 0;
			wrFile[i].Pointer = 0;
		};

		StartTime = timeGetTime();

		for (unsigned int i = 0; i < (BlocksCount / OpNumber); i++)
		{
			//���������
			for (unsigned int j = 0; j < OpNumber; j++)
			{
				rdFile[j].Offset = BaseOffset + j * BlockSize;

				if (!ReadFileEx(hdl1, MassBuffer[j], BlockSize, &rdFile[j], &ReadDone))
				{
					std::cout << "������ ��� ���������� ����� " << cPath1 << "." << std::endl;
					std::cin >> BlockSize;
					return;
				};
			};

			while (Done < OpNumber)
				SleepEx(INFINITE, TRUE);

			Done = 0;

			//����������
			for (unsigned int j = 0; j < OpNumber; j++)
			{
				wrFile[j].Offset = BaseOffset + j * BlockSize;

				if (!WriteFileEx(hdl2, MassBuffer[j], BlockSize, &wrFile[j], &WriteDone))
				{
					std::cout << "������ ��� ������ ����� " << cPath2 << "." << std::endl;
					std::cin >> BlockSize;
					return;
				};
			};

			while (Done < OpNumber)
				SleepEx(INFINITE, TRUE);

			Done = 0;

			BaseOffset += OpNumber * BlockSize;
		};
		break;
	default:
		std::cout << "������! ������� �������� ����� ��������������� �������� �����/������." << std::endl;
	};
	StartTime = (timeGetTime() - StartTime);
	std::cout << "����� ����������� �����: " << StartTime << " ��." << std::endl;
	std::cin >> BlockSize;

	CloseHandle(hdl1);
	CloseHandle(hdl2);
};

