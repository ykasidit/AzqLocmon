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

#ifndef CAzqBtGPSReader_H_
#define CAzqBtGPSReader_H_

#include <e32std.h>
#include <btmanclient.h>
#include <btextnotifiers.h>
#include <es_sock.h>
#include <in_sock.h>
#include <bt_sock.h>

#include "AzqLocationReader.h"
#include "CallbackTimer.h"

class CAzqBtGPSReader : public CAzqLocationReader
{
public:
	CAzqBtGPSReader(MBtGPSReaderObserver& aObserver);
	virtual ~CAzqBtGPSReader();
	void ConstructL();

	void DoCancel();
	void RunL();

	TInt			speed, heading;

	void StartL();

	///////////////////CCallbackTimer callback
	static TInt TimerExpired(TAny* caller);
	/////////////////////////////////
	static void ConvertFromDDDMM_MMMM_TO_DD_DDDDD(TDesC& src,TDes& dest);

private:





	TBuf8<2048> line;
	TBuf8<1024> data;




	RHostResolver iHostResolver;
	TInquirySockAddr iInquirySockAddr;
	TNameEntry iNameEntry;

	RSocket iSendingSocket;
	RSocketServ iSocketServer;
	RArray<TBTDevAddr> iBtGPSArray;

	TSockXfrLength iReadDataLength;
	TBool iAlreadySyncGPSTime;
	TInt iWaitGPSReadyRetry;

	void Cleanup();

	TTime iLineHeadReceivedTime;

	void ResetBtTimer();
	CCallbackTimer* iCallbackTimer;

};

#endif /*CAzqBtGPSReader_H_*/
