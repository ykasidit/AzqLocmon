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

#ifndef AZQLOCATIONREADER_
#define AZQLOCATIONREADER_

#include <e32std.h>
#include <e32base.h>

#include <lbsposition.h>



_LIT8(ZERO, "NA");
_LIT(ZERO16, "NA");

_LIT(KGetGPSPositionFailedStr,"GPS not ready, re-trying...");

class TGPSData
{
	public:

	TGPSData();

	TPtrC8  POS_UTC;
	TPtrC8  POS_STAT;
	TPtrC8	LATORI;
	TBuf8<15>  LAT;
	TPtrC8  LAT_REF;
	TPtrC8	LONORI;
	TBuf8<15>  LON;
	TPtrC8  LON_REF;
	TPtrC8  SPD;
	TPtrC8  HDG;
	TPtrC8  DATE;
	TPtrC8  MAG_VAR;
	TPtrC8  MAG_REF;
	TPtrC8  CC;

	TBool ParseGPSInput(TDesC8& data,TTime startRecvDataStamp,TTimeIntervalMicroSeconds& returnMicrosecsUsed);
	static void ConvertFromDDDMM_MMMM_TO_DD_DDDDD(TDes8& target, TDesC8& src);
};

class TAzqGPSData
{
	public:

	TBuf<32> iLat;
	TBuf<32> iLon;

	void Reset();
};

class MBtGPSReaderObserver
{
	public:
	virtual void OnGPSStateUpdate(const TDesC& state, TAzqGPSData& aGPSData)=0;
};


class CAzqLocationReader : public CActive
{
public:

	void SyncTimeWhenValid();//sets a flag
	void CancelSyncTimeWhenValid();//unsets a flag
	virtual void StartL()=0;

	enum TBtGPSReaderState
	{
		ENoState,
		EInquiringDevices,
		EConnecting,
		EReading,
		EReadComplete
	};


	TBtGPSReaderState GetState();
	TAzqGPSData iGPSData;

protected:
	CAzqLocationReader(MBtGPSReaderObserver& aObserver);


	TBool iSyncTimeWhenValid;

	TBtGPSReaderState iState;

	MBtGPSReaderObserver &iObserver;
};




#endif /*AZQLOCATIONREADER_*/
