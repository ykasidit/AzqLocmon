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


#include "AzqBtGPSReader.h"

#include <aknnotewrappers.h>

#include "AzenqosEngineUtils.h"


_LIT(RFCOMM, "RFCOMM");
_LIT8(KGPS, "GPS");

#define KY2K 2000

const TInt KONEHUNDREDTHOUSAND 	= 100000;
const TInt KTENTHOUSAND 		= 10000;
const TInt KMaxWaitGPSReadyRetries = 90;
const TInt KBTGPSTimeout = 45000000;//microsecs
const TInt KBtGPSMajorClass = 0x1f;



CAzqBtGPSReader::CAzqBtGPSReader(MBtGPSReaderObserver& aObserver) : CAzqLocationReader(aObserver)
{
}

CAzqBtGPSReader::~CAzqBtGPSReader()
{
	Cancel();//that would close iSendingSocket if it was opened...

	if(iCallbackTimer)
		iCallbackTimer->Cancel();
	delete iCallbackTimer;

	iSocketServer.Close();

	iBtGPSArray.Reset();
	iBtGPSArray.Close();
}

TInt CAzqBtGPSReader::TimerExpired(TAny* caller)
	{
		CAzqBtGPSReader* that = (CAzqBtGPSReader*) caller;
		that->Cancel();
		that->iState = ENoState;
		_LIT(KGPSStateUpdate,"BT GPS operation timed-out");
		that->iGPSData.Reset();
		that->iObserver.OnGPSStateUpdate(KGPSStateUpdate,that->iGPSData );

		return 0;
	}

void CAzqBtGPSReader::ResetBtTimer()
	{
		iCallbackTimer->Cancel();
		iCallbackTimer->After(KBTGPSTimeout);
	}

void CAzqBtGPSReader::RunL()
{

	iCallbackTimer->Cancel();

	switch(iState)
	{
		case ENoState:
		break;

		case EInquiringDevices:
			{
				if( iStatus !=KErrNone)
					{
						iHostResolver.Close();

						if(iStatus == KErrHostResNoMoreResults)
							{

	    							{
	    							iGPSData.Reset();
									_LIT(KGPSStateUpdate,"BT GPS Search finished...");
									iObserver.OnGPSStateUpdate(KGPSStateUpdate,iGPSData );
										}

						    //choose and connect to BT GPS modules
						    if(iBtGPSArray.Count()>0)
						    	{
						    		User::LeaveIfError(iSendingSocket.Open(iSocketServer,RFCOMM));
		    						TBTSockAddr address;
		    						address.SetBTAddr(iBtGPSArray[0]);
		    						// GPS devices usually use port 1 as data channel
		    						// so we don't have to query it
		    						address.SetPort(1);

		    							{
		    							iGPSData.Reset();
		    							iState = EConnecting;
										_LIT(KGPSStateUpdate,"Trying to connect to first device");
										iObserver.OnGPSStateUpdate(KGPSStateUpdate,iGPSData );
											}


		    						iSendingSocket.Connect(address, iStatus);
		    						SetActive();
		    						ResetBtTimer();

						    	}
						    else
						    	{

						    		{
						    		iGPSData.Reset();
						    		iState = ENoState;
									_LIT(KGPSStateUpdate,"No BT GPS. Retry press 9");
									iObserver.OnGPSStateUpdate(KGPSStateUpdate,iGPSData );
									}


						    	}

							}
						else
							{
								{
								iGPSData.Reset();
								iState = ENoState;
								_LIT(KGPSStateUpdate,"Search failed. Retry press 9");
								iObserver.OnGPSStateUpdate(KGPSStateUpdate,iGPSData );
									}



							}
					}
				else
					{

					TInquirySockAddr &addr = TInquirySockAddr::Cast(iNameEntry().iAddr);

					if(addr.MajorClassOfDevice() == KBtGPSMajorClass)
						{
							//add to array...
							iBtGPSArray.AppendL(addr.BTAddr());

								{
								iGPSData.Reset();
							_LIT(KGPSStateUpdate,"Found %d BT GPS, Searching...");
							TBuf<128> buf;
							buf.Format(KGPSStateUpdate, iBtGPSArray.Count());
							iObserver.OnGPSStateUpdate(buf,iGPSData );
								}
						}

					    ///////////get next device
						iState = EInquiringDevices;
						iHostResolver.Next(iNameEntry, iStatus);
						SetActive();
						ResetBtTimer();
						///////////////////////////////////
					}
			}
		break;

		case EConnecting:
			{
				if( iStatus !=KErrNone)
					{
						//remove previous device from array
						if(iBtGPSArray.Count()>0)
							{
							iBtGPSArray.Remove(0);
							}

						//if more gps devices remaining to try
						if(iBtGPSArray.Count()>0)
							{
								//socket alraedy opened...

								TBTSockAddr address;
								address.SetBTAddr(iBtGPSArray[0]);
								// GPS devices usually use port 1 as data channel
								// so we don't have to query it
								address.SetPort(1);

								iState = EConnecting;

									{
									iGPSData.Reset();
									_LIT(KGPSStateUpdate,"Connecting next device");
									iObserver.OnGPSStateUpdate(KGPSStateUpdate,iGPSData );
										}

								iSendingSocket.Connect(address, iStatus);
								SetActive();
								ResetBtTimer();
							}
						else//finished list
							{
								{
								iGPSData.Reset();
								iState = ENoState;

								_LIT(KGPSStateUpdate,"Connect GPS failed. Retry press 9");
								iObserver.OnGPSStateUpdate(KGPSStateUpdate,iGPSData );
									}

								iSendingSocket.Close();
							}
					}
				else
					{
						iState = EReading;
							{
							_LIT(KGPSStateUpdate,"Reading GPS data...");
							iObserver.OnGPSStateUpdate(KGPSStateUpdate,iGPSData );
								}
						iWaitGPSReadyRetry = 0; //only related if iSyncTimeWhenValid is ETrue
						line.Zero();
						iLineHeadReceivedTime = 0;//just to make sure it's fresh
						iSendingSocket.RecvOneOrMore(data, 0, iStatus, iReadDataLength);
						SetActive();
						ResetBtTimer();
					}
			}
		break;

		case EReading:
			{

				if( iStatus !=KErrNone)
					{

						{
						iGPSData.Reset();
						iState = ENoState;

						_LIT(KGPSStateUpdate,"Read GPS failed. Retry press 9");
						iObserver.OnGPSStateUpdate(KGPSStateUpdate,iGPSData );
							}


						iSendingSocket.Close();
						return;
					}

				iState = EReadComplete;

				iGPSData.Reset();//prepare for new data

				int rpos = 0;

				while ((rpos = data.Locate('\r')) != KErrNotFound)
				{
					TPtrC8 leftr(0,0);
					leftr.Set(data.Left(rpos));

					TInt leftrlen = leftr.Length();

					if(leftrlen + line.Length() < line.MaxLength())
						{
							if(line.Length()==0)
								iLineHeadReceivedTime.HomeTime();
							line.Append(leftr);
						}
					else
					{
						line.Zero();

						if(leftrlen + line.Length() < line.MaxLength())
							{
								//if(line.Length()==0) sure it is, see above lines
									iLineHeadReceivedTime.HomeTime();
								line.Append(leftr);
							}
					}

					if(rpos+1<data.Length())
						{
							data.Copy(data.Mid(rpos+1));

							if(data[0] == '\n')
								data.Delete(0,1);
						}
					else
						{
							data.Zero();
						}



					TGPSData gpsdata;
					TTimeIntervalMicroSeconds proctime;

					if(iLineHeadReceivedTime == TTime(0))//just in case...
						{
						iLineHeadReceivedTime.HomeTime();
						CAknWarningNote* informationNote = new (ELeave) CAknWarningNote(ETrue);
						//informationNote->SetTimeout(CAknNoteDialog::EShortTimeout);
						_LIT(KConfirmText,"Warning: No read-offset time correction");
						informationNote->ExecuteLD(KConfirmText);
						}

					if(gpsdata.ParseGPSInput(line,iLineHeadReceivedTime,proctime))
					{


						if((gpsdata.POS_STAT.Length() == 0) || (gpsdata.POS_STAT.Length()>0 && gpsdata.POS_STAT[0]!='A'))//if data NOT valid
							{
							///////////////BEGIN parsed but NOT valid data

							if(iSyncTimeWhenValid)//if the NetMonview is in the "Awaiting Fresh GPS Params State for the first time of session - needs time sync"
								{
									if(iWaitGPSReadyRetry>=KMaxWaitGPSReadyRetries)
									{
										//max retries finished, notify as in EReadcomplete state as set by above
										_LIT(KGPSStateUpdate,"GPS Connected but Not Ready");
										iObserver.OnGPSStateUpdate(KGPSStateUpdate,iGPSData );
										iSyncTimeWhenValid = EFalse;
									}
									else
									{
									//don't notify as in EReadcomplete state yet...
									iState = EReading;
									iWaitGPSReadyRetry++;
									TBuf<64> buf;
									_LIT(KGPSStateUpdateFormat,"GPS NotReady: TimeSync retry %d/%d");

									buf.Format(KGPSStateUpdateFormat,iWaitGPSReadyRetry,KMaxWaitGPSReadyRetries);

									iObserver.OnGPSStateUpdate(buf,iGPSData );
									}
								}
							else
								{
									_LIT(KGPSStateUpdate,"GPS Connected but Not Ready");
									iObserver.OnGPSStateUpdate(KGPSStateUpdate,iGPSData );
								}




									//latdeg = londeg = latmin = lonmin = latsec = lonsec = 0;
									line.Zero();


									if(data.Length()>0)
										{
											if(line.Length() + data.Length() < line.MaxLength())
												{
												iLineHeadReceivedTime.HomeTime();
												line.Append(data);
												}
											else
												{
													line.Zero();
													if(line.Length() + data.Length() < line.MaxLength())
														{
														//line is zero len from line.Zero() above
														iLineHeadReceivedTime.HomeTime();
														line.Append(data);
														}
												}
										}

									data.Zero();

									iState = EReading;
									iSendingSocket.RecvOneOrMore(data, 0, iStatus, iReadDataLength);
									SetActive();
									ResetBtTimer();
									return;
									///////////////END not valid
								}

						////////// boyond here is the VALID case


						////////////set device's UTC time
						if(iSyncTimeWhenValid)
							{

										TTime devTime;
										devTime.HomeTime();
										TDateTime dt = devTime.DateTime();


										if(gpsdata.POS_UTC.Length() < 6 || gpsdata.DATE.Length() < 6)
										{
											//line not ready/full... do nothing
										}
										else
										{

											//All NMEA chars are ASCII

											dt.SetHour((gpsdata.POS_UTC[0]-48)*10+(gpsdata.POS_UTC[1]-48));
											dt.SetMinute((gpsdata.POS_UTC[2]-48)*10+(gpsdata.POS_UTC[3]-48));
											dt.SetSecond((gpsdata.POS_UTC[4]-48)*10+(gpsdata.POS_UTC[5]-48));


											dt.SetDay((gpsdata.DATE[0]-48)*10+(gpsdata.DATE[1]-48) -1);

											dt.SetMonth(TMonth((gpsdata.DATE[2]-48)*10+(gpsdata.DATE[3]-48) -1));

											dt.SetYear((gpsdata.DATE[4]-48)*10+(gpsdata.DATE[5]-48)+KY2K);



											if(gpsdata.POS_UTC.Length()>9 && gpsdata.POS_UTC.Find(_L8("."))>0)
											{
												dt.SetMicroSecond((gpsdata.POS_UTC[7]-48)*KONEHUNDREDTHOUSAND+(gpsdata.POS_UTC[8]-48)*KTENTHOUSAND+(gpsdata.POS_UTC[9]-48)*1000);
											}

											devTime = dt;
											devTime += proctime;
#ifdef EKA2
											User::SetUTCTime(devTime);
#else
											//TODO: detect timezone and add accordingly
											TTime hometime(devTime);
											hometime += TTimeIntervalHours(7);
											User::SetHomeTime(hometime);
#endif


											iSyncTimeWhenValid = EFalse;

											CAknConfirmationNote* informationNote = new (ELeave) CAknConfirmationNote(EFalse);
											//informationNote->SetTimeout(CAknNoteDialog::EShortTimeout);
											_LIT(KConfirmText,"GPS TimeSync Successful");
											informationNote->ExecuteLD(KConfirmText);
											//TODO: report to scheduler that device time has changed? OR let scheduler detect device time change?


										}

							}
						/////////////////////////

							//SET LAT LON values...
							TBuf<1> ref;


							if(gpsdata.LAT.Length() < iGPSData.iLat.MaxLength()   && gpsdata.LAT_REF.Length()==1)
								{
							iGPSData.iLat.Copy(gpsdata.LAT);
							ref.Zero();
							ref.Copy(gpsdata.LAT_REF);
							iGPSData.iLat+=ref;
								}

							if(gpsdata.LON.Length() < iGPSData.iLon.MaxLength() && gpsdata.LON_REF.Length()==1)
								{
							iGPSData.iLon.Copy(gpsdata.LON);
							ref.Zero();
							ref.Copy(gpsdata.LON_REF);
							iGPSData.iLon+=ref;
								}






						_LIT(KGPSStateUpdate,"Connected to GPS");
						iObserver.OnGPSStateUpdate(KGPSStateUpdate,iGPSData );


					}//END if gps parsed


					line.Zero();
				}


				if(data.Length()>0)
					{
						if(line.Length() + data.Length() < line.MaxLength())
							{
								if(line.Length()==0) //if not appending data to previous line
									iLineHeadReceivedTime.HomeTime();
								line.Append(data);
							}
						else
							{
								line.Zero();
								if(line.Length() + data.Length() < line.MaxLength())
									{
										//if(line.Length()==0) sure it is, see above lines
											iLineHeadReceivedTime.HomeTime();
										line.Append(data);
									}
							}
					}

				data.Zero();

				iState = EReading;
				iSendingSocket.RecvOneOrMore(data, 0, iStatus, iReadDataLength);
				SetActive();
				ResetBtTimer();
			}
			break;
	}


}


void CAzqBtGPSReader::DoCancel()
{
	iCallbackTimer->Cancel();

	switch(iState)
	{

		case ENoState:
		break;


		case EInquiringDevices:
		iHostResolver.Cancel();
		iHostResolver.Close();
		break;


		case EConnecting:
			iSendingSocket.CancelConnect();
			iSendingSocket.Close();
		break;


		case EReading:
			iSendingSocket.CancelRead();
			iSendingSocket.Close();
		break;

		case EReadComplete:
			iSendingSocket.Close();
		break;

	}



	iGPSData.Reset();
	iState = ENoState;


}
void CAzqBtGPSReader::StartL()
	{

		if(iState != ENoState)
			{
				/*{
				_LIT(KGPSStateUpdate,"BT GPS Engine busy...");
				iObserver.OnGPSStateUpdate(KGPSStateUpdate,iGPSData );
					}*/

				User::Leave(KErrNotReady);
			}

		// 1. Connect to the socket server
		_LIT(KBTLM, "BTLinkManager");

		TProtocolDesc pInfo;
		TProtocolName pName(KBTLM);

		User::LeaveIfError(iSocketServer.FindProtocol(pName,pInfo));

		// 2. Create and initialise an RHostResolver
		iHostResolver.Close();//if it was already opened by a previous try;
		User::LeaveIfError(iHostResolver.Open(iSocketServer,pInfo.iAddrFamily,pInfo.iProtocol));

		// 3. Set up a discovery query and start it
		iInquirySockAddr.SetIAC(KGIAC);
		//KHostResInquiry | KHostResName | KHostResIgnoreCache
		iInquirySockAddr.SetAction(KHostResInquiry | KHostResIgnoreCache);
		iInquirySockAddr.SetMajorClassOfDevice(KBtGPSMajorClass);

		iBtGPSArray.Reset();
		iGPSData.Reset();

		iState = EInquiringDevices;

			{
		_LIT(KGPSStateUpdate,"Searching for BT GPS...");
		iObserver.OnGPSStateUpdate(KGPSStateUpdate,iGPSData );
			}

		iHostResolver.GetByAddress(iInquirySockAddr, iNameEntry, iStatus);
		SetActive();
		ResetBtTimer();

	}

void CAzqBtGPSReader::ConstructL()
{
	TBTDeviceResponseParamsPckg resultPckg;

	CActiveScheduler::Add(this);

	User::LeaveIfError(iSocketServer.Connect());

	TCallBack cb(TimerExpired,this);
	iCallbackTimer = CCallbackTimer::NewL(EPriorityHigh,cb);

	/*// 1. Create a notifier
	RNotifier notif;
	User::LeaveIfError(notif.Connect());

	TBuf8<32> gpsid;//TODO: load from file

	if (gpsid.Length() == 0)
	{
		state = 0;
		// 2. Start the device selection plug-in
		TBTDeviceSelectionParams selectionFilter;
		TUUID targetServiceClass(0x2345);
		selectionFilter.SetUUID(targetServiceClass);
		TBTDeviceSelectionParamsPckg pckg(selectionFilter);
		TRequestStatus status;
		notif.StartNotifierAndGetResponse(status,
KDeviceSelectionNotifierUid, pckg, resultPckg);
		User::After(2000000);

		// 3. Extract device name if it was returned
		User::WaitForRequest(status);



		if (status.Int() == KErrNone)
		{
			User::LeaveIfError(iSendingSocket.Open(iSocketServer,
RFCOMM));
			TBTSockAddr address;
			gpsid.Copy(resultPckg().BDAddr().Des());
			address.SetBTAddr(resultPckg().BDAddr());
			// GPS devices usually use port 1 as data channel
			// so we don't have to query it
			address.SetPort(1);

			state = 1;
			iSendingSocket.Connect(address, iStatus);
			SetActive();
		}
		else
		{
			//CHainMAppView::Static()->Notification(EGPSNotFound);
		}
	}
	else
	{
		User::LeaveIfError(iSocketServer.Connect());
		User::LeaveIfError(iSendingSocket.Open(iSocketServer, RFCOMM));

		TBTSockAddr address;
		TBTDevAddr a(gpsid);
		address.SetBTAddr(a);
		address.SetPort(1);

		state = 1;
		iSendingSocket.Connect(address, iStatus);
		SetActive();
	}*/
}




