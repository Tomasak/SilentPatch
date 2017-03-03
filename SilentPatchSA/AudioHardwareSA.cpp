#include "StdAfxSA.h"
#include "AudioHardwareSA.h"

bool			CAEDataStream::m_bUseNewStruct;

static void* CAEDataStream__Initialise = AddressByVersion<void*>(0x4DC2B0, 0x4DC7A0, 0x4E7550);
WRAPPER bool CAEDataStream::Initialise() { VARJMP(CAEDataStream__Initialise); }

unsigned int			CAEStreamingDecoder::nMallocRefCount = 0;

void*			pMalloc = nullptr;
unsigned int	nBlockSize = 0;
unsigned int	nLastMallocSize = 0;

DWORD CAEDataStreamOld::Seek(LONG nToSeek, DWORD nPoint)
{
	LARGE_INTEGER filePosition;
	switch ( nPoint )
	{
	case FILE_BEGIN:
		filePosition.QuadPart = nToSeek + dwStartPosition;
		break;
	case FILE_END:
		nPoint = FILE_BEGIN;
		filePosition.QuadPart = dwStartPosition + dwLength - nToSeek;
		break;
	default:
		filePosition.QuadPart = nToSeek;
		break;
	}

	SetFilePointerEx(hHandle, filePosition, &filePosition, nPoint);
	dwCurrentPosition = filePosition.LowPart;

	return dwCurrentPosition - dwStartPosition;
}

DWORD CAEDataStreamOld::FillBuffer(void* pBuf, DWORD nLen)
{
	ReadFile(hHandle, pBuf, nLen, &nLen, nullptr);
	dwCurrentPosition += nLen;
	return nLen;
}

DWORD CAEDataStreamNew::Seek(LONG nToSeek, DWORD nPoint)
{
	LARGE_INTEGER filePosition;
	switch ( nPoint )
	{
	case FILE_BEGIN:
		filePosition.QuadPart = nToSeek + dwStartPosition;
		break;
	case FILE_END:
		nPoint = FILE_BEGIN;
		filePosition.QuadPart = dwStartPosition + dwLength - nToSeek;
		break;
	default:
		filePosition.QuadPart = nToSeek;
		break;
	}

	SetFilePointerEx(hHandle, filePosition, &filePosition, nPoint);
	dwCurrentPosition = filePosition.LowPart;

	return dwCurrentPosition - dwStartPosition;
}

DWORD CAEDataStreamNew::FillBuffer(void* pBuf, DWORD nLen)
{
	ReadFile(hHandle, pBuf, nLen, &nLen, nullptr);
	dwCurrentPosition += nLen;
	return nLen;
}

CAEStreamingDecoder::~CAEStreamingDecoder()
{
	if ( CAEDataStream::IsNew() )
		delete reinterpret_cast<CAEDataStreamNew*>(pStream);
	else
		delete reinterpret_cast<CAEDataStreamOld*>(pStream);

	if ( --nMallocRefCount == 0 )
	{
		operator delete(pMalloc);
		pMalloc = nullptr;

		nLastMallocSize = 0;
	}
}