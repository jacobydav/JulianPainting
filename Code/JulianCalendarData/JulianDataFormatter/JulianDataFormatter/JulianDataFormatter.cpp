// JulianDataFormatter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

//The number of columns in the CSV file that will be loaded
#define NUM_COLS 32
//The number of columns containing time/position data for motors 2 & 3
#define NUM_TIME_COLS 13
//CSV Input data format:
//First line is column headers
//Each row is 1 day.
#define CI_DATE			0	//Column 1  - date
#define CI_MSG			1	//Column 2  - Message for the day
#define CI_M1Ind		2	//Column 3  - Motor 1 Index (Winter, Spring, Summer, Fall)
#define CI_M1Pos		3	//Column 4  - Motor 1 Position
#define CI_M2Ind		4	//Column 5  - Motor 2 Index (Birds, Little Dipper, Fireworks, Star, Sun, Blank)
#define CI_M2Pos_1200AM	5	//Column 6  - Motor 2 12:00 AM Position
#define CI_M2Pos_0530AM	6	//Column 7  - Motor 2  5:30 AM Position
#define CI_M2Pos_0600AM	7	//Column 8  - Motor 2  6:00 AM Position
#define CI_M2Pos_0630AM	8	//Column 9  - Motor 2  6:30 AM Position
#define CI_M2Pos_0700AM	9	//Column 10 - Motor 2  7:00 AM Position
#define CI_M2Pos_0730AM	10	//Column 11 - Motor 2  7:30 AM Position
#define CI_M2Pos_0800AM	11	//Column 12 - Motor 2  8:00 AM Position
#define CI_M2Pos_0530PM	12	//Column 13 - Motor 2  5:30 PM Position
#define CI_M2Pos_0600PM	13	//Column 14 - Motor 2  6:00 PM Position
#define CI_M2Pos_0630PM	14	//Column 15 - Motor 2  6:30 PM Position
#define CI_M2Pos_0700PM	15	//Column 16 - Motor 2  7:00 PM Position
#define CI_M2Pos_0730PM	16	//Column 17 - Motor 2  7:30 PM Position
#define CI_M2Pos_0800PM	17	//Column 18 - Motor 2  8:00 PM Position
#define CI_M3Ind		18	//Column 19 - Motor 3 Index (Full, DyingHalf, DyingCrescent, NewMoon, BabyCrescent, BabyHalf)
#define CI_M3Pos_1200AM	19	//Column 20 - Motor 3 12:00 AM Position
#define CI_M3Pos_0530AM	20	//Column 21 - Motor 3  5:30 AM Position
#define CI_M3Pos_0600AM	21	//Column 22 - Motor 3  6:00 AM Position
#define CI_M3Pos_0630AM	22	//Column 23 - Motor 3  6:30 AM Position
#define CI_M3Pos_0700AM	23	//Column 24 - Motor 3  7:00 AM Position
#define CI_M3Pos_0730AM	24	//Column 25 - Motor 3  7:30 AM Position
#define CI_M3Pos_0800AM	25	//Column 26 - Motor 3  8:00 AM Position
#define CI_M3Pos_0530PM	26	//Column 27 - Motor 3  5:30 PM Position
#define CI_M3Pos_0600PM	27	//Column 28 - Motor 3  6:00 PM Position
#define CI_M3Pos_0630PM	28	//Column 29 - Motor 3  6:30 PM Position
#define CI_M3Pos_0700PM	29	//Column 30 - Motor 3  7:00 PM Position
#define CI_M3Pos_0730PM	30	//Column 31 - Motor 3  7:30 PM Position
#define CI_M2Pos_0800PM	31	//Column 32 - Motor 3  8:00 PM Position

std::string inDataFilename = "JulianRawData.csv";
std::string outDataFilename = "Julian.txt";
std::string hourValArray[] = {"00","05","06","06","07","07","08","17","18","18","19","19","20"};
std::string minuteValArray[] = {"00","30","00","30","00","30","00","30","00","30","00","30","00"};

//Motor 1 map position names to position index
std::map<std::string, std::string> motor1PosMap;
//Motor 2 map position names to position index
std::map<std::string, std::string> motor2PosMap;
//Motor 3 map position names to position index
std::map<std::string, std::string> motor3PosMap;

//Convert the string in the input data at the specified column
//to the output data format
std::string GetOutputData(int rowIndex, std::vector<std::string> inDataVec);

int _tmain(int argc, _TCHAR* argv[])
{
	//Motor 1 map position names to position index
	motor1PosMap.insert(std::pair<std::string, std::string>("Fall", "1"));
	motor1PosMap.insert(std::pair<std::string, std::string>("Winter", "2"));
	motor1PosMap.insert(std::pair<std::string, std::string>("SumSpr", "3"));	
	//Motor 2 map position names to position index
	motor2PosMap.insert(std::pair<std::string, std::string>("Sun", "1"));
	motor2PosMap.insert(std::pair<std::string, std::string>("Star", "2"));
	motor2PosMap.insert(std::pair<std::string, std::string>("Fireworks", "3"));
	motor2PosMap.insert(std::pair<std::string, std::string>("LittleDipper", "4"));
	motor2PosMap.insert(std::pair<std::string, std::string>("Birds", "5"));
	motor2PosMap.insert(std::pair<std::string, std::string>("Blank", "6"));
	//Motor 3 map position names to position index	
	motor3PosMap.insert(std::pair<std::string, std::string>("DyingCrescent", "1"));
	motor3PosMap.insert(std::pair<std::string, std::string>("DyingHalf", "2"));
	motor3PosMap.insert(std::pair<std::string, std::string>("Full", "3"));
	motor3PosMap.insert(std::pair<std::string, std::string>("BabyHalf", "4"));	
	motor3PosMap.insert(std::pair<std::string, std::string>("BabyCrescent", "5"));
	motor3PosMap.insert(std::pair<std::string, std::string>("NewMoon", "6"));

	//Open input data file
	std::cout<<"Reading input file: "<<inDataFilename<<std::endl;
	std::ifstream inFile;
	inFile.open(inDataFilename.c_str());
	if(!inFile.is_open())
	{
		std::cout<<"Could not load input file."<<std::endl;
		std::cout<<"Press Enter to exit."<<std::endl;
		std::cin.get();		
		return 0;
	}

	//Create storage for output data
	std::vector< std::string > outDataVec;

	//Begin reading data
	std::string headerRow;
	std::getline(inFile, headerRow);   //Read the header row
	int currRow=0;
	int currCharIndex = 0;
	std::string dataRow;
	std::getline(inFile, dataRow);
	while(inFile)
	{
		std::vector<std::string> rowDataVec;
		std::vector<std::string> rawInDataVec;
		for(int colIndex=0; colIndex<NUM_COLS; colIndex++)
		{
			int loc = dataRow.find(",",currCharIndex);
			std::string subStr = dataRow.substr(currCharIndex, loc-currCharIndex);
			rawInDataVec.push_back(subStr);
			currCharIndex = loc + 1;
		}

		//Testing begin
		std::cout<<rawInDataVec[0]<<std::endl;
		//for(int n=0; n<rawInDataVec.size(); n++)
		//{
		//	std::cout<<n<<"    "<<rawInDataVec[n]<<std::endl;
		//}
		//Testing end
		std::string outRowStr = GetOutputData(currRow, rawInDataVec);
		outDataVec.push_back(outRowStr);
		currRow++;
		std::getline(inFile, dataRow);
	}

	inFile.close();

	//Write the data to file
	std::cout<<"Writing output file: "<<outDataFilename<<std::endl;
	std::ofstream outFile;
	outFile.open(outDataFilename.c_str());

	for(int i=0;i<outDataVec.size();i++)
	{
		outFile<<outDataVec[i]<<std::endl;
	}

	outFile.close();

	std::cout<<"Complete. Press enter to exit."<<std::endl;
	//Wait for key press before closing.
	std::cin.ignore();

	return 0;
}

//Convert the string in the input data
//to the output data format
std::string GetOutputData(int rowIndex, std::vector<std::string> inDataVec)
{
	std::string outStr="";
	//Day Index
	std::stringstream ss;
	ss << (rowIndex+1);
	std::string intStr = ss.str();
	outStr.append(intStr);
	outStr.append(",");
	//Day message
	outStr.append(inDataVec[CI_MSG]);
	outStr.append(",");
	//Motor 1
	//Motor 1 has only position per day
	std::string motor1PosStr = motor1PosMap[inDataVec[CI_M1Pos]];
	outStr.append("M1");
	outStr.append(motor1PosStr);
	outStr.append(hourValArray[0]);
	outStr.append(minuteValArray[0]);
	outStr.append(",");
	//Motor 2
	//First read the position value that starts at midnight
	//Then find the time where the position changes and create an entry
	//for the new position at that time.
	int timeStrIndex = 0;
	int firstTimeCol = CI_M2Ind + 1;		//Position at midnight
	std::string motor2InitPosStr = motor2PosMap[inDataVec[firstTimeCol]];
	outStr.append("M2");
	outStr.append(motor2InitPosStr);
	outStr.append(hourValArray[timeStrIndex]);
	outStr.append(minuteValArray[timeStrIndex]);
	outStr.append(",");
	timeStrIndex++;
	int startCol = CI_M2Ind + 2;
	int stopCol = startCol+NUM_TIME_COLS-1;
	std::string currPosStr = motor2InitPosStr;
	for(int colIndex=startCol;colIndex<stopCol;colIndex++)
	{
		//Find where the position value changes
		if(motor2PosMap[inDataVec[colIndex]]!=currPosStr)
		{
			std::string motor2PosStr = motor2PosMap[inDataVec[colIndex]];
			outStr.append("M2");
			outStr.append(motor2PosStr);
			outStr.append(hourValArray[timeStrIndex]);
			outStr.append(minuteValArray[timeStrIndex]);
			outStr.append(",");
			currPosStr = motor2PosStr;
		}
		timeStrIndex++;
	}
	//Motor 3
	//First read the position value that starts at midnight
	//Then find the time where the position changes and create an entry
	//for the new position at that time.
	timeStrIndex = 0;
	firstTimeCol = CI_M3Ind + 1;		//Position at midnight
	std::string motor3InitPosStr = motor3PosMap[inDataVec[firstTimeCol]];
	outStr.append("M3");
	outStr.append(motor3InitPosStr);
	outStr.append(hourValArray[timeStrIndex]);
	outStr.append(minuteValArray[timeStrIndex]);
	timeStrIndex++;
	startCol = CI_M3Ind + 2;
	stopCol = startCol+NUM_TIME_COLS-1;
	currPosStr = motor3InitPosStr;
	for(int colIndex=startCol;colIndex<stopCol;colIndex++)
	{
		//Find where the position value changes
		if(motor3PosMap[inDataVec[colIndex]]!=currPosStr)
		{
			outStr.append(",");
			std::string motor3PosStr = motor3PosMap[inDataVec[colIndex]];
			outStr.append("M3");
			outStr.append(motor3PosStr);
			outStr.append(hourValArray[timeStrIndex]);
			outStr.append(minuteValArray[timeStrIndex]);
			
			currPosStr = motor3PosStr;
		}
		timeStrIndex++;
	}
	return outStr;
}