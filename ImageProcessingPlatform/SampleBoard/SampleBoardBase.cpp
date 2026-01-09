#include "SampleBoardBase.h"
#include "PZMedical.h"
#include "DRTECH.h"

ISampleBoardBase* ISampleBoardBase::GetSampleBoard(int SampleBoardType)
{
	switch (SampleBoardType)
	{
	case 0:
	{
		return new CPZMedical();
	}
	case 1:
	{
		return new CDRTECH();
	}
	default:
	{
		return nullptr;
	}
	}
}