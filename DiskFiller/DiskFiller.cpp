// DiskFiller.cpp : Defines the entry point for the console application.
//
//FAT32: Maximum number of files in a single folder: 65,534
//NTFS: Maximum number of files in a single folder : 4, 294, 967, 295

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <string>
#include "windows.h"

using namespace std;

#define BUFFERSIZE 8192
#define DIRECTORYNAME "DiskFill\\"
#define DIRECTORYNAMEL  L"DiskFill\\"


void fillBuff(unsigned char * buff, unsigned char value, size_t s)
{
	size_t i = 0;

	for (i = 0; i < s; i++)
	{
		buff[i] = value;
	}
}

void fileFill(char * drive, unsigned char * buff, int number, int loopCount)
{
	ofstream file;
	int i;
	string name = drive;
	name += DIRECTORYNAME + to_string(number) + ".bin";

	file.open(name, ios::binary);

	for (i = 0; i < loopCount; i++)
	{
		file << buff;
	}

	file.close();
}


//this fuction converts a wchar_t string to a char string. 
//caller is responsible for freeing allocated memory
char * wcharTochar(wchar_t * source)
{
	size_t len, i;
	for (len = 0; source[len] != 0; len++)
		;
	len++;
	char * newC = (char *)malloc(len);
	for (i = 0; i < len - 1; i++)
	{
		newC[i] = (char)source[i];
	}
	newC[len - 1] = 0;
	return newC;
}

//this fuction converts a char string to a wchar_t string. 
//caller is responsible for freeing allocated memory
wchar_t * charTowchar(char * source)
{
	size_t len, i;
	for (len = 0; source[len] != 0; len++)
		;
	len++;
	wchar_t * newC = (wchar_t *)malloc(sizeof(wchar_t) * len);
	for (i = 0; i < len - 1; i++)
	{
		newC[i] = source[i];
	}
	newC[len - 1] = 0;
	return newC;
}

int strToint(char * source)
{
	int val = 0;
	size_t len = strlen(source);

	for (size_t i = 0; i < len; i++)
	{
		if (source[i] >= '0' && source[i] <= '9')
		{
			//val = val * 10 + source[i] - '0';
			val += (source[i] - '0') * (int)pow(10, len - i - 1);
		}
		else
		{
			return -1;
		}
	}

	return val;
}

void doWork(char * drive, int times)
{
	unsigned char * buff = NULL;
	wchar_t dirNameW[1024] = { (wchar_t)0 };
	wchar_t fileNameMask[1024] = { (wchar_t)0 };
	int count;
	ULARGE_INTEGER freeBytes;
	ULARGE_INTEGER prevBytes;

	{
		wchar_t * temp = charTowchar(drive);
		wcscat_s(dirNameW, temp);
		wcscat_s(dirNameW, DIRECTORYNAMEL);
		wcscat_s(fileNameMask, temp);
		wcscat_s(fileNameMask, DIRECTORYNAMEL);
		wcscat_s(fileNameMask, L"*");
		free(temp);
	}

	for (count = 0; count < times; count++)
	{
		int num = 0,
			numSame = 0,
			dataEnter = 1024;
		unsigned char value = (0 == count % 2) ? 0xAA : 0x55;

		prevBytes.HighPart = 0;
		prevBytes.LowPart = -1;

		buff = (unsigned char *)malloc(BUFFERSIZE);

		fillBuff(buff, value, BUFFERSIZE);

		//create Dir, if already exists, clear all files in directory
		int test = CreateDirectory(dirNameW, NULL);
		if (test == 0)
		{
			DWORD error = GetLastError();

			if (error == ERROR_ALREADY_EXISTS)
			{
				//Delete all files in directory
				WIN32_FIND_DATA ffd;
				HANDLE hfind = INVALID_HANDLE_VALUE;
				wchar_t fullpath[1024];

				// Find the first file in the directory.
				hfind = FindFirstFile(fileNameMask, &ffd);
				if (INVALID_HANDLE_VALUE != hfind)
				{
					int te;
					do
					{
						fullpath[0] = 0;
						WIN32_FIND_DATA temp = ffd;
						te = FindNextFile(hfind, &temp);
						wcscat_s(fullpath, dirNameW);
						wcscat_s(fullpath, L"\\");
						wcscat_s(fullpath, ffd.cFileName);

						DeleteFile(fullpath);
						ffd = temp;
					} while (te != 0);
				}


			}
			else
			{
				cout << "Improper file position.\n";
				break;
			}
		}
		do
		{
			//fill directory with 1 Mb files
			fileFill(drive, buff, num, dataEnter);
			num++;

			//print out every 10000 cycles to show activity
			if (0 == num % 10000)
				cout << num << " iterations complete\n";

			if (0 == num % 100)
			{// every 1000 iterations, check if disk is full
				if (SHGetDiskFreeSpaceEx(charTowchar(drive) , &freeBytes, NULL, NULL) != 0)
				{
					if (freeBytes.HighPart == 0)
					{//if high part of space is zero, check low part
						if (freeBytes.LowPart == 0)
						{//disk is full
							cout << "Disk Full\n";
							break;
						}
						else if (freeBytes.LowPart != prevBytes.LowPart)
						{//low part has changed
							numSame = 0;
							prevBytes.LowPart = freeBytes.LowPart;
						}
						else
						{//low part has not changed
							numSame++;
							if (numSame > 5)
							{
								if (dataEnter == 1)
								{
									//low part has not changed multiple times, disk is full. 
									cout << "Disk nearly Full with " << freeBytes.LowPart << " Bytes Free\n";
									break;
								}
								else if (dataEnter == 1024)
								{//reduce from 16 Mb to 1 Mb
									dataEnter = 128;
								}
								else
								{// reduce to 8 kb
									dataEnter = 1;
								}
							}
						}
					}
				}
			}
			//loop until break
		} while (true);

		free(buff);
	}//for
}

void displayUsage()
{
	cout << "\n\n Disk Fill Utility\n";
	cout << "  This utility writes alternating 10101010 and 01010101 patterns\n  to free hard drive sectors.\n\n";
	cout << "  Usage: DiskFiller.exe [Drive] [Reps]\n";
	cout << "  Where\n";
	cout << "    Drive is the drive to fill, defaults to C:\\\n";
	cout << "    Reps  is the number of times to fill the disk, defaults to 7\n\n";
}


int main(int argc, char** argv)
{
	int times = 2;
	char drive [4]= "C:\\";

	if (argc > 3)
	{
		displayUsage();
		exit(2);
	}

	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] >= '0'  && argv[i][0] <= '9')
		{
			times = strToint(argv[i]);
			cout << "loop " << times << " times\n";
		}
		else if ((argv[i][0] >= 'A' && argv[i][0] <= 'Z') || (argv[i][0] >= 'a' && argv[i][0] <= 'z'))
		{
			drive[0] = argv[i][0];
			cout << drive << "\n";
		}
		else
		{
			displayUsage();
			exit(2);
		}
	}
	
	doWork(drive, times);

	return 0;
}

