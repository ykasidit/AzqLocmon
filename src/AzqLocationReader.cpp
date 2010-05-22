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


#include "AzqLocationReader.h"
#include "AzenqosEngineUtils.h"
#include <e32math.h>

const TInt KONEHUNDREDTHOUSAND 	= 100000;
const TInt KTENTHOUSAND 		= 10000;
const TInt KMaxWaitGPSReadyRetries = 10;


void Parse(TDesC8 &data, TInt &deg, TInt &min, TInt &sec, TInt32 &whole)
{
	TLex8 lex(data);

	lex.Val(deg);
	lex.Inc();
	lex.Val(min);
	sec = min * 60;
	int l = (data.Length() - data.Find(_L8(".")));
	int x = 1;
	while (l-- > 0)
		x *= 10;
	sec /= x/100;

	min = deg % 100;
	deg /= 100;

	whole = deg * 65536 + (min * 65536 / 60) + (sec * 65536 / 36000);
}



TGPSData::TGPSData():POS_UTC(NULL,0)
						,  POS_STAT(NULL,0)
						,  LATORI(NULL,0)
						,  LAT_REF(NULL,0)
						,  LONORI(NULL,0)
						,  LON_REF(NULL,0)
						,  SPD(NULL,0)
						,  HDG(NULL,0)
						,  DATE(NULL,0)
						,  MAG_VAR(NULL,0)
						,  MAG_REF(NULL,0)
						,  CC(NULL,0)
{

}

void TGPSData::ConvertFromDDDMM_MMMM_TO_DD_DDDDD(TDes8& target, TDesC8& src)
{
	TLex8 lex(src);


								TReal reallat=0;

								lex.Val(reallat);

								if(reallat!=0)
								{
									TReal remainder;
									TReal r100 = 100;
									Math::Mod(remainder,reallat,r100);
									remainder/=(TReal)60;
									reallat = (TReal)(((TInt)reallat)/100);
									reallat+= remainder;

									target.Format(_L8("%.05f"),reallat);
								}

}

TBool TGPSData::ParseGPSInput(TDesC8& indata,TTime startRecvDataStamp,TTimeIntervalMicroSeconds& returnMicrosecsUsed)
{
				_LIT8(KGPRMCHeader,"$GPRMC");
				TInt pos = indata.Find(KGPRMCHeader);

				if(pos<0)
					return EFalse;

				TPtrC8 data(NULL,0);
				data.Set(indata.Right(indata.Length()-pos-1));

				if(pos >=0/* && indata.Length() >= (pos+50)  BECAUSE the CSV parser would check anyway...*/)
				{
						TTime now;
						now.HomeTime();
						returnMicrosecsUsed = now.MicroSecondsFrom(startRecvDataStamp);
						TInt64 proctime = returnMicrosecsUsed.Int64();
						TInt64 pos64(pos);
						TInt64 len64(indata.Length());
						proctime *= pos64;
						proctime /= len64; //make ratio estimation of real processing/transfer time;

						TPtrC8 remainder(NULL,0);

						//GARMIN:
						//$GPRMC,022857,V,1346.8867,N,10040.4624,E,,,220806,000.4,
						//$GPRMC,022059,V,1346.8867,N,10040.4624,E,,,220806,000.4,W*75

						//NON-GARMIN:
						//$GPRMC,042204.024,V,0000.0000,N,00000.0000,E,0.000000,,260506,,*0B

	    	/*			POS_UTC  - UTC of position. Hours, minutes and seconds. (hhmmss)
						  POS_STAT - Position status. (A = Data valid, V = Data invalid)
						  LAT      - Latitude (llll.ll)
						  LAT_REF  - Latitude direction. (N = North, S = South)
						  LON      - Longitude (yyyyy.yy)
						  LON_REF  - Longitude direction (E = East, W = West)
						  SPD      - Speed over ground. (knots) (x.x)
						  HDG      - Heading/track made good (degrees True) (x.x)
						  DATE     - Date (ddmmyy)
						  MAG_VAR  - Magnetic variation (degrees) (x.x)
						  MAG_REF  - Magnetic variation (E = East, W = West)
						  CC       - Checksum (optional)

						  */

						if(TAzenqosEngineUtils::TokenizeCSV8(data,POS_UTC,remainder))//clear the $GPRMC out of the str
						if(TAzenqosEngineUtils::TokenizeCSV8(remainder,POS_UTC,remainder))
						if(TAzenqosEngineUtils::TokenizeCSV8(remainder,POS_STAT,remainder))
						if(TAzenqosEngineUtils::TokenizeCSV8(remainder,LATORI,remainder))
						if(TAzenqosEngineUtils::TokenizeCSV8(remainder,LAT_REF,remainder))
						if(TAzenqosEngineUtils::TokenizeCSV8(remainder,LONORI,remainder))
						if(TAzenqosEngineUtils::TokenizeCSV8(remainder,LON_REF,remainder))
						if(TAzenqosEngineUtils::TokenizeCSV8(remainder,SPD,remainder))
						if(TAzenqosEngineUtils::TokenizeCSV8(remainder,HDG,remainder))
						if(TAzenqosEngineUtils::TokenizeCSV8(remainder,DATE,remainder))
						if(TAzenqosEngineUtils::TokenizeCSV8(remainder,MAG_VAR,remainder))
						if(TAzenqosEngineUtils::TokenizeCSV8(remainder,MAG_REF,remainder))
						if(TAzenqosEngineUtils::TokenizeCSV8(remainder,CC,remainder))	//last one must return false but the string is supposed to be longer
						{
							if(POS_STAT.Length() ==1  && POS_STAT[0] == 'A')
							{
								//convert LAT and LON from ddd mm.mmmm to ddd.ddddd
								ConvertFromDDDMM_MMMM_TO_DD_DDDDD(LAT,LATORI);
								ConvertFromDDDMM_MMMM_TO_DD_DDDDD(LON,LONORI);

								TTime then;
								then.HomeTime();
								TTimeIntervalMicroSeconds otherproctime = then.MicroSecondsFrom(now);
								TInt64 otherproctime64;
								otherproctime64 = otherproctime.Int64();
								proctime +=otherproctime64;
								returnMicrosecsUsed = proctime;


								//return ETrue; return false only if parse failed... let user check POS_STAT himself
							}
/*								TBuf<64> msg;
								msg.Copy(POS_STAT);
							 CAknConfirmationNote* informationNote = new (ELeave) CAknConfirmationNote(ETrue);
					   	informationNote->ExecuteLD(msg);
								msg.Copy(POS_UTC);
					   	informationNote = new (ELeave) CAknConfirmationNote(ETrue);
					   	informationNote->ExecuteLD(msg);		*/

							return ETrue;



						}
						else
						{
							if(POS_STAT.Length() ==1  && POS_STAT[0] == 'A')
								{
								//convert LAT and LON from ddd mm.mmmm to ddd.ddddd
								ConvertFromDDDMM_MMMM_TO_DD_DDDDD(LAT,LATORI);
								ConvertFromDDDMM_MMMM_TO_DD_DDDDD(LON,LONORI);

								TTime then;
								then.HomeTime();
								TTimeIntervalMicroSeconds otherproctime = then.MicroSecondsFrom(now);
								TInt64 otherproctime64;
								otherproctime64 = otherproctime.Int64();
								proctime +=otherproctime64;
								returnMicrosecsUsed = proctime;

								//return ETrue; return false only if parse failed... let user check POS_STAT himself

								}

							return ETrue;//data not valid

						}
				}

				return EFalse;
}



void TAzqGPSData::Reset()
{

	 iLat = ZERO16;
	 iLon = ZERO16;


}



CAzqLocationReader::CAzqLocationReader(MBtGPSReaderObserver& aObserver) : CActive(EPriorityLow),iObserver(aObserver)
{
iGPSData.Reset();
iSyncTimeWhenValid = EFalse;
}

void CAzqLocationReader::SyncTimeWhenValid()//sets a flag
	{
	iSyncTimeWhenValid= ETrue;

	}

void CAzqLocationReader::CancelSyncTimeWhenValid()//unsets a flag
	{
	iSyncTimeWhenValid= EFalse;
	}

CAzqLocationReader::TBtGPSReaderState CAzqLocationReader::GetState()
	{
		return iState;
	}
