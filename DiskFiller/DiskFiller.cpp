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
#define DIRECTORYNAME "C:\\DiskFill\\"
#define DIRECTORYNAMEL L"C:\\DiskFill"
#define FILENAMEMASK  L"C:\\DiskFill\\*"



void fillBuff(unsigned char * buff, unsigned char value, size_t s)
{
	size_t i = 0;

	for (i = 0; i < s; i++)
	{
		buff[i] = value;
	}
}

void fileFill(unsigned char * buff, int number, int loopCount)
{
	ofstream file;
	int i;
	string name = DIRECTORYNAME + to_string(number) + ".bin";

	file.open(name, ios::binary);

	for (i = 0; i < loopCount; i++)
	{
		file << buff;
	}

	file.close();
}

int main(int argc, char** argv)
{
	unsigned char * buff = NULL;
	int times = 2;
	int count;
	ULARGE_INTEGER freeBytes;
	ULARGE_INTEGER prevBytes;

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
		int test = CreateDirectory(DIRECTORYNAMEL, NULL);
		if (test == 0)
		{
			DWORD error = GetLastError();

			if (error == ERROR_ALREADY_EXISTS)
			{
				//TODO: Delete all files in directory
				WIN32_FIND_DATA ffd;
				HANDLE hfind = INVALID_HANDLE_VALUE;
				wchar_t fullpath [1024];

				// Find the first file in the directory.
				hfind = FindFirstFile(FILENAMEMASK, &ffd);
				if (INVALID_HANDLE_VALUE != hfind)
				{
					int te;
					do
					{
						fullpath[0] = 0;
						WIN32_FIND_DATA temp = ffd;
						te = FindNextFile(hfind, &temp);
						wcscat_s(fullpath, DIRECTORYNAMEL);
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
			fileFill(buff, num, dataEnter);
			num++;
		
			/*
			//fill directory with 8 kb files
			fileFill(buff, num, 1);
			num++;
			cout << "Disk Full\n";
			break;
			*/

			//print out every 10000 cycles to show activity
			if (0 == num % 10000)
				cout << num << " iterations complete\n";
			
			if (0 == num % 100)
			{// every 1000 iterations, check if disk is full
				if (SHGetDiskFreeSpaceEx(L"C:\\", &freeBytes, NULL, NULL) != 0)
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

	return 0;
}

