/* Copyright (c) 2010 Benjamin Dobell, Glass Echidna
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.*/

// C/C++ Standard Library
#include <algorithm>
#include <map>
#include <stdio.h>
#include <string>
#include <memory>

// Heimdall
#include "BridgeManager.h"
#include "DeviceInfoPacket.h"
#include "DeviceInfoResponse.h"
#include "EndModemFileTransferPacket.h"
#include "EndPhoneFileTransferPacket.h"
#include "InterfaceManager.h"
#include "PitData.h"

using namespace std;
using namespace Heimdall;

// Known partitions
enum EKnownPartition
{
	kKnownPartitionPit = 0,
	kKnownPartitionPrimaryBootloader,
	kKnownPartitionSecondaryBootloader,
	kKnownPartitionSecondaryBootloaderBackup,
	kKnownPartitionRecovery,
	kKnownPartitionKernel,
	kKnownPartitionParam,
	kKnownPartitionEfs,
	kKnownPartitionModem,
	kKnownPartitionFactoryFs,
	kKnownPartitionCache,
	kKnownPartitionDatabaseData,
	kKnownPartitionNormalBoot,
	kKnownPartitionSystem,
	kKnownPartitionUserData,
	kKnownPartitionFota,
	kKnownPartitionHidden,
	kKnownPartitionMovinand,
	kKnownPartitionData,
	kKnownPartitionUms,
	kKnownPartitionEmmc,

	kKnownPartitionCount
};

vector<const char *> knownPartitionNames[kKnownPartitionCount];

struct PartitionNameFileData
{
	string partitionName;
	FILE *file;
	int partitionIdentifier;

	PartitionNameFileData(const char *partitionName, FILE *file, int partitionIdentifier)
	{
		this->partitionName = partitionName;
		this->file = file;
		this->partitionIdentifier = partitionIdentifier;
	}
};

void initialiseKnownPartitionNames(void)
{
	knownPartitionNames[kKnownPartitionPit].push_back("PIT");
	knownPartitionNames[kKnownPartitionFactoryFs].push_back("FACTORYFS");
	knownPartitionNames[kKnownPartitionCache].push_back("CACHE");
	knownPartitionNames[kKnownPartitionDatabaseData].push_back("DBDATAFS");

	knownPartitionNames[kKnownPartitionPrimaryBootloader].push_back("IBL+PBL");
	knownPartitionNames[kKnownPartitionPrimaryBootloader].push_back("BOOT");

	knownPartitionNames[kKnownPartitionSecondaryBootloader].push_back("SBL");
	knownPartitionNames[kKnownPartitionSecondaryBootloader].push_back("SBL1");

	knownPartitionNames[kKnownPartitionSecondaryBootloaderBackup].push_back("SBL2");
	knownPartitionNames[kKnownPartitionParam].push_back("PARAM");
	knownPartitionNames[kKnownPartitionKernel].push_back("KERNEL");
	knownPartitionNames[kKnownPartitionRecovery].push_back("RECOVERY");
	knownPartitionNames[kKnownPartitionEfs].push_back("EFS");
	knownPartitionNames[kKnownPartitionModem].push_back("MODEM");

	knownPartitionNames[kKnownPartitionNormalBoot].push_back("NORMALBOOT");
	knownPartitionNames[kKnownPartitionSystem].push_back("SYSTEM");
	knownPartitionNames[kKnownPartitionUserData].push_back("USERDATA");
	knownPartitionNames[kKnownPartitionFota].push_back("FOTA");
	knownPartitionNames[kKnownPartitionHidden].push_back("HIDDEN");
	knownPartitionNames[kKnownPartitionMovinand].push_back("MOVINAND");
	knownPartitionNames[kKnownPartitionData].push_back("DATAFS");
	knownPartitionNames[kKnownPartitionUms].push_back("UMS.EN");
	knownPartitionNames[kKnownPartitionEmmc].push_back("GANG");

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-pit", kKnownPartitionPit));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("pit", kKnownPartitionPit));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-factoryfs", kKnownPartitionFactoryFs));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("fs", kKnownPartitionFactoryFs));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-cache", kKnownPartitionCache));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("cache", kKnownPartitionCache));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-dbdata", kKnownPartitionDatabaseData));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("db", kKnownPartitionDatabaseData));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-primary-boot", kKnownPartitionPrimaryBootloader));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("boot", kKnownPartitionPrimaryBootloader));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-secondary-boot", kKnownPartitionSecondaryBootloader));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("sbl", kKnownPartitionSecondaryBootloader));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-secondary-boot-backup", kKnownPartitionSecondaryBootloaderBackup));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("sbl2", kKnownPartitionSecondaryBootloaderBackup));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-param", kKnownPartitionParam));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("param", kKnownPartitionParam));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-kernel", kKnownPartitionKernel));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("z", kKnownPartitionKernel));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-recovery", kKnownPartitionRecovery));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("rec", kKnownPartitionRecovery));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-efs", kKnownPartitionEfs));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("efs", kKnownPartitionEfs));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-modem", kKnownPartitionModem));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("m", kKnownPartitionModem));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-system", kKnownPartitionSystem));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("sys", kKnownPartitionSystem));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-normal-boot", kKnownPartitionNormalBoot));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("norm", kKnownPartitionNormalBoot));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-user-data", kKnownPartitionUserData));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("udata", kKnownPartitionUserData));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-hidden", kKnownPartitionHidden));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("hide", kKnownPartitionHidden));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-movinand", kKnownPartitionMovinand));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("nand", kKnownPartitionMovinand));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-fota", kKnownPartitionFota));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("fota", kKnownPartitionFota));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-data", kKnownPartitionData));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("data", kKnownPartitionData));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-ums", kKnownPartitionUms));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("ums", kKnownPartitionUms));

	InterfaceManager::flashArgumentMap.insert(pair<string, int>("-emmc", kKnownPartitionEmmc));
	InterfaceManager::flashArgumentMap.insert(pair<string, int>("emmc", kKnownPartitionEmmc));

}

bool openFiles(const map<string, string>& argumentMap, map<string, FILE *>& argumentFileMap)
{
	map<string, string>::const_iterator it = argumentMap.begin();

	for (it = argumentMap.begin(); it != argumentMap.end(); it++)
	{
		bool isFileArgument = false;

		int partitionIndex = atoi(it->first.substr(it->first.find_first_not_of('-')).c_str());

		// Was the argument a partition index?
		if (partitionIndex > 0 || it->first.compare("-0") == 0)
		{
			isFileArgument = true;
		}
		else
		{
			// The argument wasn't a partition index, check if it's a known partition name.
			for (int knownPartition = 0; knownPartition < kKnownPartitionCount; knownPartition++)
			{
				if (it->first.compare(InterfaceManager::flashArgumentNames[InterfaceManager::kFlashArgPit + knownPartition]) == 0)
				{
					isFileArgument = true;
					break;
				}
			}
		}

		if (!isFileArgument)
			continue;

		pair<string, FILE *> argumentFilePair;
		argumentFilePair.first = it->first;
		argumentFilePair.second = fopen(it->second.c_str(), "rb");

		if (!argumentFilePair.second)
		{
			InterfaceManager::PrintError("Failed to open file \"%s\"\n", it->second.c_str());
			return (false);
		}

		argumentFileMap.insert(argumentFilePair);
	}

	return (true);
}

bool mapFilesToPartitions(const map<string, FILE *>& argumentFileMap, const PitData *pitData, map<unsigned int, PartitionNameFileData>& partitionFileMap)
{
	partitionFileMap.clear();
	map<string, FILE *>::const_iterator it = argumentFileMap.begin();
	map<string, FILE *>::const_iterator itEnd = argumentFileMap.end();
	for (; it != itEnd; ++it)
	{
		int partitionIndex = atoi(it->first.substr(it->first.find_first_not_of('-')).c_str());

		const PitEntry *pitEntry = nullptr;
		int knownPartition = -1;

		// Was the argument a partition index?
		if (partitionIndex > 0 || it->first.compare("-0") == 0)
		{
			pitEntry = pitData->FindEntry(partitionIndex);
			knownPartition = partitionIndex;
		}
		else
		{
			// The argument wasn't a partition index, so it must be a known partition name.

			map<string, int>::const_iterator itKnownMap = InterfaceManager::flashArgumentMap.begin();
			map<string, int>::const_iterator itKnownMapEnd = InterfaceManager::flashArgumentMap.end();

			for ( ; itKnownMap != itKnownMapEnd; ++itKnownMap)
			{
				if (it->first.compare(itKnownMap->first) == 0)
				{
					knownPartition = itKnownMap->second;
					break;
				}
			}

			// Check for the partition in the PIT file using all known names.
			for (unsigned int i = 0; i < knownPartitionNames[knownPartition].size(); i++)
			{
				pitEntry = pitData->FindEntry(knownPartitionNames[knownPartition][i]);

				if (pitEntry)
					break;
			}
		}

		if (!pitEntry)
		{
			InterfaceManager::PrintError("Partition corresponding to %s argument could not be located\n", it->first.c_str());
			return (false);
		}

		PartitionNameFileData partitionNameFileData(pitEntry->GetPartitionName(), it->second, pitEntry->GetPartitionIdentifier());
		//partitionFileMap.insert(pair<unsigned int, PartitionNameFilePair>(pitEntry->GetPartitionIdentifier(), partitionNameFilePair));
		try
		{
			partitionFileMap.insert(pair<unsigned int, PartitionNameFileData>(knownPartition, partitionNameFileData));
		}
		catch (...)
		{
			InterfaceManager::PrintError("Unable to allocate partition %d. Possible duplicate.\n", knownPartition);
			return false;
		}
	}

	return (true);
}

void closeFiles(map<string, FILE *> argumentfileMap)
{
	for (map<string, FILE *>::iterator it = argumentfileMap.begin(); it != argumentfileMap.end(); it++)
		fclose(it->second);

	argumentfileMap.clear();
}

int downloadPitFile(BridgeManager *bridgeManager, vector<unsigned char> *pitBuffer)
{
	if (!pitBuffer)
		return (-1);

	InterfaceManager::Print("Downloading device's PIT file...\n");

	int devicePitFileSize = bridgeManager->ReceivePitFile(pitBuffer);

	if (pitBuffer->empty())
	{
		InterfaceManager::PrintError("Failed to download PIT file!\n");

		return (-1);
	}

	InterfaceManager::Print("PIT file download sucessful\n\n");
	return devicePitFileSize;
}

bool flashFile(BridgeManager *bridgeManager, unsigned int partitionIndex, const char *partitionName, FILE *file)
{
	// PIT files need to be handled differently, try determine if the partition we're flashing to is a PIT partition.
	bool isPit = false;

	for (unsigned int i = 0; i < knownPartitionNames[kKnownPartitionPit].size(); i++)
	{
		if (strcmp(partitionName, knownPartitionNames[kKnownPartitionPit][i]) == 0)
		{
			isPit = true;
			break;
		}
	}

	if (isPit)
	{
		InterfaceManager::Print("Uploading %s\n", partitionName);

		if (bridgeManager->SendPitFile(file))
		{
			InterfaceManager::Print("%s upload successful\n", partitionName);
			return (true);
		}
		else
		{
			InterfaceManager::PrintError("%s upload failed!\n", partitionName);
			return (false);
		}
	}
	else
	{
		// Modems need to be handled differently, try determine if the partition we're flashing to is a modem partition.
		bool isModem = false;

		for (unsigned int i = 0; i < knownPartitionNames[kKnownPartitionModem].size(); i++)
		{
			if (strcmp(partitionName, knownPartitionNames[kKnownPartitionModem][i]) == 0)
			{
				isModem = true;
				break;
			}
		}

		if (isModem)
		{			
			InterfaceManager::Print("Uploading %s\n", partitionName);

			//if (bridgeManager->SendFile(file, EndPhoneFileTransferPacket::kDestinationPhone,    // <-- Kies method. WARNING: Doesn't work on Galaxy Tab!
			//	EndPhoneFileTransferPacket::kFileModem))
			if (bridgeManager->SendFile(file, EndModemFileTransferPacket::kDestinationModem))     // <-- Odin method
			{
				InterfaceManager::Print("%s upload successful\n", partitionName);
				return (true);
			}
			else
			{
				InterfaceManager::PrintError("%s upload failed!\n", partitionName);
				return (false);
			}
		}
		else
		{
			// We're uploading to a phone partition
			InterfaceManager::Print("Uploading %s\n", partitionName);

			if (bridgeManager->SendFile(file, EndPhoneFileTransferPacket::kDestinationPhone, partitionIndex))
			{
				InterfaceManager::Print("%s upload successful\n", partitionName);
				return (true);
			}
			else
			{
				InterfaceManager::PrintError("%s upload failed!\n", partitionName);
				return (false);
			}
		}
	}

	return (true);
}

bool prepareDeviceToFlash(BridgeManager *bridgeManager, int totalBytesToFlash)
{
	bool success = false;
	// ---------- GET DEVICE INFORMATION ----------

	DeviceInfoPacket deviceInfoPacket(DeviceInfoPacket::kUnknown1);
	success = bridgeManager->SendPacket(&deviceInfoPacket);


	if (!success)
	{
		InterfaceManager::PrintError("Failed to send device info packet!\nFailed Request: kUnknown1\n");
		return (false);
	}

	DeviceInfoResponse deviceInfoResponse;
	success = bridgeManager->ReceivePacket(&deviceInfoResponse);
	int unknown = deviceInfoResponse.GetUnknown();

	if (!success)
	{
		InterfaceManager::PrintError("Failed to receive device info response!\n");
		return (false);
	}

	// 131072 for Galaxy S II, 0 for other devices.
	if (unknown != 0 && unknown != 131072)
	{
		InterfaceManager::PrintError("Unexpected device info response!\nExpected: 0\nReceived:%i\n", unknown);
		return (false);
	}

	// -------------------- KIES DOESN'T DO THIS --------------------
	DeviceInfoPacket deviceInfoPacket2(DeviceInfoPacket::kUnknown2);
	success = bridgeManager->SendPacket(&deviceInfoPacket2);

	if (!success)
	{
		InterfaceManager::PrintError("Failed to send device info packet!\nFailed Request: kUnknown2\n");
		return (false);
	}	

	success = bridgeManager->ReceivePacket(&deviceInfoResponse);
	unknown = deviceInfoResponse.GetUnknown();

	if (!success)
	{
		InterfaceManager::PrintError("Failed to receive device info response!\n");
		return (false);
	}

	// TODO: Work out what this value is... it has been either 180 or 0 for Galaxy S phones, 3 on the Galaxy Tab, 190 for SHW-M110S.
	if (unknown != 180 && unknown != 0 && unknown != 3 && unknown != 190)
	{
		InterfaceManager::PrintError("Unexpected device info response!\nExpected: 180, 0 or 3\nReceived:%i\n", unknown);
		return (false);
	}
	// --------------------------------------------------------------

	DeviceInfoPacket deviceInfoPacket3(DeviceInfoPacket::kTotalBytes, totalBytesToFlash);
	success = bridgeManager->SendPacket(&deviceInfoPacket3);

	if (!success)
	{
		InterfaceManager::PrintError("Failed to send total bytes device info packet!\n");
		return (false);
	}

	success = bridgeManager->ReceivePacket(&deviceInfoResponse);
	unknown = deviceInfoResponse.GetUnknown();

	if (!success)
	{
		InterfaceManager::PrintError("Failed to receive device info response!\n");
		return (false);
	}

	if (unknown != 0)
	{
		InterfaceManager::PrintError("Unexpected device info response!\nExpected: 0\nReceived:%i\n", unknown);
		return (false);
	}

	// -----------------------------------------------------

	return true;
}

bool attemptFlash(BridgeManager *bridgeManager, map<string, FILE *> argumentFileMap, bool repartition)
{
	bool success = false;

	int totalBytes = 0;
	for (map<string, FILE *>::const_iterator it = argumentFileMap.begin(); it != argumentFileMap.end(); it++)
	{
		fseek(it->second, 0, SEEK_END);
		totalBytes += ftell(it->second);
		rewind(it->second);
	}

	success = prepareDeviceToFlash(bridgeManager, totalBytes);
	if (!success)
	{
		InterfaceManager::PrintError("Failed to get device information and prepare flashing\n");
		return (false);
	}

	PitData pitData;
	FILE *localPitFile = nullptr;

	if (repartition)
	{
		// If we're repartitioning then we need to unpack the information from the specified PIT file.

		map<string, FILE *>::iterator it = argumentFileMap.find(InterfaceManager::flashArgumentNames[InterfaceManager::kFlashArgPit]);

		// This shouldn't ever happen due to early checks, but we'll check again just in case...
		if (it == argumentFileMap.end())
		{
			InterfaceManager::PrintError("Attempt was made to repartition without specifying a PIT file!\n");
			return (false);
		}

		localPitFile = it->second;

		// Load the local pit file into memory.
		vector<unsigned char> pitFileBuffer(4096, 0);

		fseek(localPitFile, 0, SEEK_END);
		long localPitFileSize = ftell(localPitFile);
		rewind(localPitFile);

		// dataRead is discarded, it's here to remove warnings.
		int dataRead = fread(&pitFileBuffer.front(), 1, localPitFileSize, localPitFile);
		rewind(localPitFile);

		pitData.Unpack(&pitFileBuffer.front());
	}
	else
	{
		// If we're not repartitioning then we need to retrieve the device's PIT file and unpack it.

		vector<unsigned char> pitFileBuffer;
		downloadPitFile(bridgeManager, &pitFileBuffer);

		pitData.Unpack(&pitFileBuffer.front());
	}

	map<unsigned int, PartitionNameFileData> partitionFileMap;

	// Map the files being flashed to partitions stored in PIT file.
	mapFilesToPartitions(argumentFileMap, &pitData, partitionFileMap);

	// If we're repartitioning then we need to flash the PIT file first.
	if (repartition)
	{
		for (map<unsigned int, PartitionNameFileData>::iterator it = partitionFileMap.begin(); it != partitionFileMap.end(); it++)
		{
			if (it->second.file == localPitFile)
			{
				if (!flashFile(bridgeManager, it->second.partitionIdentifier, it->second.partitionName.c_str(), it->second.file))
					return (false);
			}
		}
	}

	// Flash all other files
	for (map<unsigned int, PartitionNameFileData>::iterator it = partitionFileMap.begin(); it != partitionFileMap.end(); it++)
	{
		if (it->second.file != localPitFile)
		{
			if (!flashFile(bridgeManager, it->second.partitionIdentifier, it->second.partitionName.c_str(), it->second.file))
				return (false);
		}
	}

	return (true);
}

int main(int argc, char **argv)
{
	map<string, string> argumentMap;
	int actionIndex;

	if (!InterfaceManager::GetArguments(argc, argv, argumentMap, &actionIndex))
	{
		Sleep(250);
		return (0);
	}

	initialiseKnownPartitionNames();

	if (actionIndex == InterfaceManager::kActionHelp)
	{
		InterfaceManager::Print(InterfaceManager::usage);
		return (0);
	}
	else if (actionIndex == InterfaceManager::kActionFlash)
	{
		if (argumentMap.find(InterfaceManager::flashArgumentNames[InterfaceManager::kFlashArgRepartition]) != argumentMap.end()
			&& argumentMap.find(InterfaceManager::flashArgumentNames[InterfaceManager::kFlashArgPit]) == argumentMap.end())
		{
			InterfaceManager::Print("If you wish to repartition then a PIT file must be specified.\n");
			return (0);
		}

		if (argumentMap.find(InterfaceManager::flashArgumentNames[InterfaceManager::kFlashArgPit]) != argumentMap.end()
			&& argumentMap.find(InterfaceManager::flashArgumentNames[InterfaceManager::kFlashArgRepartition]) == argumentMap.end())
		{
			InterfaceManager::Print("A PIT file should only be used when repartitioning.\n");
			return (0);
		}
	}
	else if (actionIndex == InterfaceManager::kActionDump)
	{
		if (argumentMap.find(InterfaceManager::dumpArgumentNames[InterfaceManager::kDumpArgOutput]) == argumentMap.end())
		{
			InterfaceManager::Print("Output file not specified.\n\n");
			InterfaceManager::Print(InterfaceManager::usage);
			return (0);
		}

		if (argumentMap.find(InterfaceManager::dumpArgumentNames[InterfaceManager::kDumpArgChipType]) == argumentMap.end())
		{
			InterfaceManager::Print("You must specify a chip type.\n\n");
			InterfaceManager::Print(InterfaceManager::usage);
			return (0);
		}

		string chipType = argumentMap.find(InterfaceManager::dumpArgumentNames[InterfaceManager::kDumpArgChipType])->second;
		if (!(chipType == "RAM" || chipType == "ram" || chipType == "NAND" || chipType == "nand"))
		{
			InterfaceManager::Print("Unknown chip type: %s.\n\n", chipType.c_str());
			InterfaceManager::Print(InterfaceManager::usage);
			return (0);
		}

		if (argumentMap.find(InterfaceManager::dumpArgumentNames[InterfaceManager::kDumpArgChipId]) == argumentMap.end())
		{
			InterfaceManager::Print("You must specify a Chip ID.\n\n");
			InterfaceManager::Print(InterfaceManager::usage);
			return (0);
		}

		int chipId = atoi(argumentMap.find(InterfaceManager::dumpArgumentNames[InterfaceManager::kDumpArgChipId])->second.c_str());
		if (chipId < 0)
		{
			InterfaceManager::Print("Chip ID must be a non-negative integer.\n");
			return (0);
		}
	}

	InterfaceManager::Print("\nHeimdall v1.2.0, Copyright (c) 2010-2011, Benjamin Dobell, Glass Echidna\n");
	InterfaceManager::Print("http://www.glassechidna.com.au\n\n");
	InterfaceManager::Print("This software is provided free of charge. Copying and redistribution is\nencouraged.\n\n");
	InterfaceManager::Print("If you appreciate this software and you would like to support future\ndevelopment please consider donating:\n");
	InterfaceManager::Print("http://www.glassechidna.com.au/donate/\n\n");
	
	Sleep(1000);

	bool verbose = argumentMap.find(InterfaceManager::commonArgumentNames[InterfaceManager::kCommonArgVerbose]) != argumentMap.end();
	bool noReboot = argumentMap.find(InterfaceManager::commonArgumentNames[InterfaceManager::kCommonArgNoReboot]) != argumentMap.end();

	int communicationDelay = BridgeManager::kCommunicationDelayDefault;
	if (argumentMap.find(InterfaceManager::commonArgumentNames[InterfaceManager::kCommonArgDelay]) != argumentMap.end())
		communicationDelay = atoi(argumentMap.find(InterfaceManager::commonArgumentNames[InterfaceManager::kCommonArgDelay])->second.c_str());

	BridgeManager bridgeManager(verbose, communicationDelay);

	if (!bridgeManager.Initialise())
	{
		return (-2);
	}

	bool success;

	switch (actionIndex)
	{
		case InterfaceManager::kActionFlash:
		{
			map<string, FILE *> argumentFileMap;

			// We open the files before doing anything else to ensure they exist.
			if (!openFiles(argumentMap, argumentFileMap))
			{
				closeFiles(argumentFileMap);

				return (0);
			}

			if (!bridgeManager.BeginSession())
			{
				closeFiles(argumentFileMap);

				return (-1);
			}

			bool repartition = argumentMap.find(InterfaceManager::flashArgumentNames[InterfaceManager::kFlashArgRepartition]) != argumentMap.end();
			success = attemptFlash(&bridgeManager, argumentFileMap, repartition);

			if (noReboot)
				success = bridgeManager.EndSession() && success;
			else
				success = bridgeManager.EndSession() && bridgeManager.RebootDevice() && success;

			closeFiles(argumentFileMap);

			break;
		}

		case InterfaceManager::kActionClosePcScreen:
		{
			if (!bridgeManager.BeginSession())
			{
				return (-1);
			}

			InterfaceManager::Print("Attempting to close connect to pc screen...\n");

			if (noReboot)
				success = bridgeManager.EndSession();
			else
				success = bridgeManager.EndSession() && bridgeManager.RebootDevice();

			if (success)
				InterfaceManager::Print("Attempt complete\n");

			break;
		}

		case InterfaceManager::kActionDump:
		{
			const char *outputFilename = argumentMap.find(InterfaceManager::dumpArgumentNames[InterfaceManager::kDumpArgOutput])->second.c_str();
			FILE *dumpFile = fopen(outputFilename, "wb");
			if (!dumpFile)
			{
				InterfaceManager::PrintError("Failed to open file \"%s\"\n", outputFilename);

				return (-1);
			}

			int chipType = 0;
			string chipTypeName = argumentMap.find(InterfaceManager::dumpArgumentNames[InterfaceManager::kDumpArgChipType])->second;
			if (chipTypeName == "NAND" || chipTypeName == "nand")
				chipType = 1;

			int chipId = atoi(argumentMap.find(InterfaceManager::dumpArgumentNames[InterfaceManager::kDumpArgChipId])->second.c_str());

			if (!bridgeManager.BeginSession())
			{
				fclose(dumpFile);

				return (-1);
			}

			success = bridgeManager.ReceiveDump(chipType, chipId, dumpFile);

			fclose(dumpFile);

			if (noReboot)
				success = bridgeManager.EndSession() && success;
			else
				success = bridgeManager.EndSession() && bridgeManager.RebootDevice() && success;

			break;
		}

		case InterfaceManager::kActionPrintPit:
		{
			if (!bridgeManager.BeginSession())
			{
				return (-1);
			}

			vector<unsigned char> devicePit;
			if (downloadPitFile(&bridgeManager, &devicePit) < -1)
			{
				if (!bridgeManager.EndSession())
					return (-1);

				if (!noReboot)
					bridgeManager.RebootDevice();

				return (-1);
			}

			PitData pitData;

			if (pitData.Unpack(&devicePit.front()))
			{
				pitData.Print();
				success = true;
			}
			else
			{
				InterfaceManager::PrintError("Failed to unpack device's PIT file!\n");
				success = false;
			}

			if (noReboot)
				success = bridgeManager.EndSession() && success;
			else
				success = bridgeManager.EndSession() && bridgeManager.RebootDevice() && success;

			break;
		}
	}

	return ((success) ? 0 : -1);
}
