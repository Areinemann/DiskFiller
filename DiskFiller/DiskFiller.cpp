/* DiskFiller.cpp : Defines the entry point for the console application.
*
* Repeatedly fills and emptys a drive with an alternating 10101010 and 01010101 patterns
*
*Author:	Alexander Reinemann
*Date:		6/25/17
*
*Developer Notes:
*	FAT32: Maximum number of files in a single folder: 65,534
*	NTFS: Maximum number of files in a single folder : 4,294,967,295
*/

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <string>
#include "windows.h"

using namespace std;

#define BUFFERSIZE				8192
#define DIRECTORY_NAME			"DiskFill\\"
#define DIRECTORY_NAME_L		L"DiskFill\\"
#define DISK_FULL_CHECK_COUNT	100
#define PROGRESS_REPORT_COUNT	10000
#define VERSION					"1.0.1"

/*
*fillBuff
*Fills the buffer with an assigned character
*
*Parameters:
*		buff	the buffer to fill
*		value	the unsigned charater to fill the buffer with
*		sz		the size of the buffer
*/
void fillBuff(unsigned char * buff, unsigned char value, size_t sz)
{
	size_t i = 0;

	for (i = 0; i < sz; i++)
	{
		buff[i] = value;
	}
}

/*
*fileWrite
*Writes data file to disk
*Parameters:
*		drive				the drive to write the files to
*		buff				the data buffer to write into the file
*		number				number used to name the file
*		bufferWriteCount	number of times to write the buffer to the file
*/
void fileWrite(char * drive, unsigned char * buff, int number, int bufferWriteCount)
{
	ofstream	file;
	int			i;
	string		name = drive;
	name += DIRECTORY_NAME + to_string(number) + ".bin";

	file.open(name, ios::binary);

	for (i = 0; i < bufferWriteCount; i++)
	{//Write data buffer to the file
		file << buff;
	}

	file.close();
}


/*
*charToWchar
*This fuction converts a null-terminated char string to a null-terminated wchar_t string. 
*The caller is responsible for freeing allocated memory.
*Parameters:
*		source	a null-terminated char sequence to convert
*Return:
*		newC	a pointer to a null-terminated wchar sequence
*/
wchar_t * charToWchar(char * source)
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


/*
*strToInt
*Convert a string of chars to a positive integer value;
*similar to atoi except it returns -1 on a non-integer character
*Parameter:
*		source	char sequence to convert to int
*Return:
*		if successful, returns the integer value
*		-1 if source contains invalid numeric characters
*/
int strToInt(char * source)
{
	int		val = 0;
	size_t	len = strlen(source);

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

/*
*fillDisk
*Fills the designated drive with alternating paterns of 10101010 and 01010101 specified number of times
*Parameters:
*		drive	the drive to fill (e.g. "C:\")
*		times	the number of times to fill and empty the drive
*/
void fillDisk(char * drive, int times)
{
	unsigned char * buff = (unsigned char *)malloc(BUFFERSIZE);
	wchar_t			dirNameW[1024] = { (wchar_t)0 };
	wchar_t			fileNameMask[1024] = { (wchar_t)0 };
	int				count;
	ULARGE_INTEGER	freeBytes;
	ULARGE_INTEGER	prevBytes;

	{
		//determine the wchar sequences for dirName and fileNameMask
		wchar_t * temp = charToWchar(drive);
		wcscat_s(dirNameW, temp);
		wcscat_s(dirNameW, DIRECTORY_NAME_L);
		wcscat_s(fileNameMask, temp);
		wcscat_s(fileNameMask, DIRECTORY_NAME_L);
		wcscat_s(fileNameMask, L"*");
		free(temp);
	}

	for (count = 0; count < times; count++)
	{//loop through the designated number of empty \  fill iterations
		int num = 0,
			numSame = 0,
			bufferWriteCount = 1024;
		unsigned char value = (0 == count % 2) ? 0xAA : 0x55;

		prevBytes.HighPart = 0;
		prevBytes.LowPart = -1;


		fillBuff(buff, value, BUFFERSIZE);

		//create Dir, if already exists, clear all files in directory
		int test = CreateDirectory(dirNameW, NULL);
		if (test == 0)
		{
			DWORD error = GetLastError();

			if (error == ERROR_ALREADY_EXISTS)
			{
				//Delete all files in directory
				WIN32_FIND_DATA fileData;
				HANDLE hfind = INVALID_HANDLE_VALUE;
				wchar_t fullpath[1024];

				// Find the first file in the directory.
				hfind = FindFirstFile(fileNameMask, &fileData);
				if (INVALID_HANDLE_VALUE != hfind)
				{
					int result;
					do
					{//loop through all files in the directory and delete them
						fullpath[0] = 0;
						WIN32_FIND_DATA temp = fileData;
						result = FindNextFile(hfind, &temp);
						wcscat_s(fullpath, dirNameW);
						wcscat_s(fullpath, L"\\");
						wcscat_s(fullpath, fileData.cFileName);

						DeleteFile(fullpath);
						fileData = temp;
					} while (result != 0);
				}
			}
			else
			{
				cout << "Unable to create directory.\n";
				break;
			}
		}
		do
		{
			//fill directory with large files
			fileWrite(drive, buff, num, bufferWriteCount);
			num++;

			//Report progress every PROGRESS_REPORT_COUNT iterations
			if (0 == num % PROGRESS_REPORT_COUNT)
				cout << num << " iterations complete\n";

			if (0 == num % DISK_FULL_CHECK_COUNT)
			{//Check if disk is full every DISK_FULL_CHECK_COUNT iterations 
				if (SHGetDiskFreeSpaceEx(charToWchar(drive) , &freeBytes, NULL, NULL) != 0)
				{//SHGetDiskFreeSpaceEx successful
					if (freeBytes.HighPart == 0)
					{//if high part of free space is zero, check low part
						if (freeBytes.LowPart == 0)
						{//disk is full
							cout << "Disk full after " << count + 1 << " loops\n";
							break;
						}
						else if (freeBytes.LowPart != prevBytes.LowPart)
						{//low part of free space has changed
							numSame = 0;
							prevBytes.LowPart = freeBytes.LowPart;
						}
						else
						{//low part of free space has not changed
							numSame++;
							if (numSame > 5)
							{//free space on the Hard Drive has not changed in 500 iterations
								if (bufferWriteCount == 1)
								{
									//low part has not changed in last 5 free space checks, disk is full. 
									cout << "Disk nearly Full with " << freeBytes.LowPart << " Bytes Free after " << count + 1 << " loops\n";
									break;
								}
								else if (bufferWriteCount == 1024)
								{//reduce file size from 8 Mb to 1 Mb
									bufferWriteCount = 128;
								}
								else
								{// reduce file size from 1 Mb to 8 Kb
									bufferWriteCount = 1;
								}
							}//free space on the Hard Drive has not changed in 500 iterations
						}//low part of free space has not changed
					}//if high part of free space is zero, check low part
				}//SHGetDiskFreeSpaceEx successful
			}//Check if disk is full every DISK_FULL_CHECK_COUNT iterations
		} while (true);//loop until break
	}//for
	free(buff);
}//fillDisk

/*
*signOn
*Displays the name and current version of the program being run
*/
void signOn()
{
	cout << "\n\n Disk Fill Utility Version " << VERSION << "\n\n";
}

/*
*displayUsage
*Display usage message describling command line parameters
*/
void displayUsage()
{
	cout << "  This utility writes alternating 10101010 and 01010101 patterns\n  to free hard drive sectors.\n\n";
	cout << "  Usage: DiskFiller.exe [Drive] [Reps]\n";
	cout << "  Where\n";
	cout << "    Drive is the drive to fill, defaults to C:\\\n";
	cout << "    Reps  is the number of times to fill the disk, defaults to 6\n\n";
}

/*
*main
*main function of the program that parses arguments and takes the apropriate actions.
*/
int main(int argc, char** argv)
{
	//default values
	int times = 6;
	char drive [4]= "C:\\";

	signOn();

	if (argc > 3)
	{//too many arguments
		displayUsage();
		return 2;
	}

	for (int i = 1; i < argc; i++)
	{//loop through command line arguments
		if (argv[i][0] >= '0'  && argv[i][0] <= '9')
		{//number of loops argument
			times = strToInt(argv[i]);
		}
		else if ((argv[i][0] >= 'A' && argv[i][0] <= 'Z') || (argv[i][0] >= 'a' && argv[i][0] <= 'z'))
		{//drive argument
			drive[0] = argv[i][0];
		}
		else
		{//incorect argument
			displayUsage();
			return 2;
		}
	}

	cout << "Loop " << times << " times on drive " << drive << "\n";
	
	if (times > 0)
		fillDisk(drive, times);

	return 0;
}//main

