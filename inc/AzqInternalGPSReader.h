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

#ifndef AZQINTERNALGPSREADER_H
#define AZQINTERNALGPSREADER_H

#ifdef EKA2

#include <e32base.h>	// For CActive, link against: euser.lib
#include <e32std.h>		// For RTimer, link against: euser.lib

#include "AzqLocationReader.h"

#include <lbs.h>
#include <lbserrors.h>



class CAzqInternalGPSReader : public CAzqLocationReader
	{
public:
	// Cancel and destroy
	virtual ~CAzqInternalGPSReader();

	// Two-phased constructor.
	static CAzqInternalGPSReader* NewL(MBtGPSReaderObserver& aObserver);

	// Two-phased constructor.
	static CAzqInternalGPSReader* NewLC(MBtGPSReaderObserver& aObserver);

public:


	void StartL();

private:
	// C++ constructor
	CAzqInternalGPSReader(MBtGPSReaderObserver& aObserver);

	// Second-phase constructor
	void ConstructL();

private:
	// From CActive
	// Handle completion
	void RunL();

	// How to cancel me
	void DoCancel();

	// Override to handle leaves from RunL(). Default implementation causes
	// the active scheduler to panic.
	TInt RunError(TInt aError);


	RPositionServer iPositionServer;
	RPositioner iPositioner;

	TPosition iPosition;

	TPositionInfo iPositionInfo;

	TPositionModuleId iPositionModuleId ;

	};
#endif

#endif // AZQINTERNALGPSREADER_H
