/*
    Copyright (C) 2010 Kasidit Yusuf.

    This file is part of AzqLocmon.

    AzqLocmon is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    AzqLocmon is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with AzqLocmon.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifdef EKA2

#include "AzqInternalGPSReader.h"
#include <aknnotewrappers.h>

#include <lbscommon.h>


const TInt KMillion = 1000000;

CAzqInternalGPSReader::CAzqInternalGPSReader(MBtGPSReaderObserver& aObserver) :
CAzqLocationReader(aObserver)
	{
	}

CAzqInternalGPSReader* CAzqInternalGPSReader::NewLC(MBtGPSReaderObserver& aObserver)
	{
	CAzqInternalGPSReader* self = new ( ELeave ) CAzqInternalGPSReader(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}

CAzqInternalGPSReader* CAzqInternalGPSReader::NewL(MBtGPSReaderObserver& aObserver)
	{
	CAzqInternalGPSReader* self = CAzqInternalGPSReader::NewLC(aObserver);
	CleanupStack::Pop(); // self;
	return self;
	}

void CAzqInternalGPSReader::ConstructL()
	{
	CActiveScheduler::Add( this); // Add to scheduler

	User::LeaveIfError(iPositionServer.Connect());


	//leave if internal gps not present

	TUint numModules=0;
	TPositionModuleInfo modInfo;
	TPositionModuleStatus modStatus;

	TBool foundModule = EFalse;

	// 2. Get the number of modules installed
	User::LeaveIfError(iPositionServer.GetNumModules(numModules));

	// 3. Iterate over the modules to get information about each module
	// 4. Get the availability of a module
	// 5. Get information about the module technology, quality etc.

	for (TUint I=0 ; I < numModules ; I++)
	 {
		 User::LeaveIfError(iPositionServer.GetModuleInfoByIndex(I, modInfo));

		 /* Check module technology type and availability
		    In this example - does the module support assisted capability
		    and is the module available? */

		 if ( modInfo.IsAvailable() && (modInfo.TechnologyType() == ( TPositionModuleInfo::ETechnologyAssisted) ) )
		 {
			  foundModule = ETrue;
			  iPositionModuleId = modInfo.ModuleId();
			  break;
		 }
	 }


	if (foundModule)
	 {

	 	User::LeaveIfError(iPositioner.Open(iPositionServer,iPositionModuleId));

	 }
	else
	 {
	    User::Leave(KErrNotFound);
	 }

	}

CAzqInternalGPSReader::~CAzqInternalGPSReader()
	{
	Cancel(); // Cancel any request, if outstanding

	iPositioner.Close();
	iPositionServer.Close();
	// Delete instance variables if any
	}



void CAzqInternalGPSReader::StartL()
	{

	Cancel();


	_LIT(KOurAppName,"AzenqosTester");
	User::LeaveIfError(iPositioner.SetRequestor(
	        CRequestor::ERequestorService, CRequestor::EFormatApplication, KOurAppName));

	TPositionUpdateOptions options;

	// Frequency of updates in microseconds
	const TTimeIntervalMicroSeconds KUpdateInterval(3*KMillion);
	// How long the application is willing to wait before timing out the request
	const TTimeIntervalMicroSeconds KTimeOut(60*KMillion);

	// The maximum acceptable age of the information in an update
	const TTimeIntervalMicroSeconds KMaxUpdateAge(2*KMillion);

	options.SetUpdateInterval(KUpdateInterval);
	options.SetUpdateTimeOut(KTimeOut);
	options.SetMaxUpdateAge(KMaxUpdateAge);
	options.SetAcceptPartialUpdates(EFalse);

	User::LeaveIfError(iPositioner.SetUpdateOptions(options));

	/* Now when the application requests location information
	it will be provided with these options */

	iState = EReading;

	iPositioner.NotifyPositionUpdate(iPositionInfo, iStatus);
	SetActive();

	}

void CAzqInternalGPSReader::DoCancel()
	{
	iPositioner.CancelRequest(EPositionerNotifyPositionUpdate);
	iState = ENoState;
	}

void CAzqInternalGPSReader::RunL()
	{
	    iState = EReadComplete;

		if(iStatus.Int() == KErrNone )
			{
				iPositionInfo.GetPosition(iPosition);

				if(iSyncTimeWhenValid)
					{
					User::SetUTCTime(iPosition.Time());

					iSyncTimeWhenValid = EFalse;

					CAknConfirmationNote* informationNote = new (ELeave) CAknConfirmationNote(EFalse);
					_LIT(KConfirmText,"GPS TimeSync Successful");
					informationNote->ExecuteLD(KConfirmText);

					}

				TAzqGPSData aGPSData;
				_LIT8(KGPSPosValFormat8,"%.05f");
				TBuf8<32> tmpdst,tmpsrc;

				///////////////////LAT
				tmpdst.Zero();
				tmpsrc.Format(KGPSPosValFormat8,iPosition.Latitude());
				//TGPSData::ConvertFromDDDMM_MMMM_TO_DD_DDDDD(tmpdst,tmpsrc);

				aGPSData.iLat.Copy(tmpsrc);

				if(iPosition.Latitude()<0)
					{
					_LIT(KSouthInd,"S");
					aGPSData.iLat += KSouthInd;
					}
				else
					{
					_LIT(KNorthInd,"N");
					aGPSData.iLat += KNorthInd;
					}
				///////////////////////

				///////////////////LON
				tmpdst.Zero();
				tmpsrc.Format(KGPSPosValFormat8,iPosition.Longitude());
				//TGPSData::ConvertFromDDDMM_MMMM_TO_DD_DDDDD(tmpdst,tmpsrc);

				aGPSData.iLon.Copy(tmpsrc);
				if(iPosition.Longitude()<0)
				{
				_LIT(KWestInd,"W");
				aGPSData.iLon += KWestInd;
				}
			else
				{
				_LIT(KEastInd,"E");
				aGPSData.iLon += KEastInd;
				}

				_LIT(KGPSStateUpdate,"Got GPS position report");
				iObserver.OnGPSStateUpdate(KGPSStateUpdate,aGPSData );
				////////////////////////

			}
		else
			{

			TAzqGPSData aGPSData;
			iObserver.OnGPSStateUpdate(KGetGPSPositionFailedStr,aGPSData );
			}



		StartL();
	}

TInt CAzqInternalGPSReader::RunError(TInt aError)
	{
	return KErrNone;
	}

#endif

