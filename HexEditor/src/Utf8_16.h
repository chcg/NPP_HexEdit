// Utf8_16.h
// Copyright (C) 2002 Scott Kirkwood
//
// Permission to use, copy, modify, distribute and sell this code
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies or
// any derived copies.  Scott Kirkwood makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.
//
// Notes: Used the UTF information I found at:
//   http://www.cl.cam.ac.uk/~mgk25/unicode.html
////////////////////////////////////////////////////////////////////////////////
// 
// Modificated 2006 Jens Lorenz
// 
// - Clean up the sources
// - Removing UCS-Bug in Utf8_Iter
// - Add convert function in Utf8_16_Write
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stdio.h>
#include <assert.h>

#ifdef _MSC_VER
#pragma warning(disable: 4514) // nreferenced inline function has been removed
#endif

class Utf8_16 {
public:
	typedef unsigned short utf16; // 16 bits
	typedef unsigned char utf8; // 8 bits
	typedef unsigned char ubyte;
	enum class encodingType {
		eUnknown,
		eUtf8,
		eUtf16BigEndian,
		eUtf16LittleEndian,  // Default on Windows
		eUtf8Plain,
		eLast
	};
	static const utf8 k_Boms[static_cast<size_t>(encodingType::eLast)][3];
};

// Reads UTF-16 and outputs UTF-8
class Utf16_Iter : public Utf8_16 {
public:
	enum class eState {
		eStart,
		e2Bytes2,
		e3Bytes2,
		e3Bytes3
	};

	Utf16_Iter();
	void reset();
	void set(const ubyte* pBuf, size_t nLen, encodingType eEncoding);
	utf8 get() const { return m_nCur; };
	void operator++();
	eState getState() { return m_eState; };
	operator bool() { return m_pRead <= m_pEnd; };

protected:
	encodingType m_eEncoding;
	eState m_eState;
	utf8 m_nCur;
	utf16 m_nCur16;
	const ubyte* m_pBuf;
	const ubyte* m_pRead;
	const ubyte* m_pEnd;
};

// Reads UTF-8 and outputs UTF-16
class Utf8_Iter : public Utf8_16 {
public:
	Utf8_Iter();
	void reset();
	void set(const ubyte* pBuf, size_t nLen, encodingType eEncoding);
	utf16 get() const {
#ifdef _DEBUG
		assert(m_eState == eState::eStart);
#endif
		return m_nCur;
	}
	bool canGet() const { return m_eState == eState::eStart; }
	void operator++();
	operator bool() { return m_pRead <= m_pEnd; }

protected:
	void swap();
	void toStart(); // Put to start state, swap bytes if necessary
	enum class eState {
		eStart,
		e2Bytes_Byte2,
		e3Bytes_Byte2,
		e3Bytes_Byte3
	};
protected:
	encodingType m_eEncoding;
	eState m_eState;
	utf16 m_nCur;
	const ubyte* m_pBuf;
	const ubyte* m_pRead;
	const ubyte* m_pEnd;
};

// Reads UTF16 and outputs UTF8
class Utf8_16_Read : public Utf8_16 {
public:
	Utf8_16_Read();
	~Utf8_16_Read();

	size_t convert(char* buf, size_t len);
	char* getNewBuf() { return reinterpret_cast<char*>(m_pNewBuf); }

	void forceEncoding(encodingType eType);
	void noBOM(void) { m_bIsBOM = false; };

	encodingType getEncoding() const { return m_eEncoding; }
protected:
	void determineEncoding();
	int isUTF8_16();
private:
	encodingType    m_eEncoding;
	ubyte*          m_pBuf;
	ubyte*          m_pNewBuf;
	size_t          m_nBufSize;
	size_t			m_nSkip;
	bool            m_bFirstRead;
	size_t          m_nLen;
	Utf16_Iter      m_Iter16;
	bool			m_bIsBOM;
};

// Read in a UTF-8 buffer and write out to UTF-16 or UTF-8
class Utf8_16_Write : public Utf8_16 {
public:
	Utf8_16_Write();
	~Utf8_16_Write();

	void setEncoding(encodingType eType);
	void disableBOM(void) { m_bIsBOM = false; };

	FILE * fopen(const char *_name, const char *_type);
	size_t fwrite(const void* p, size_t _size);
	void   fclose();

	size_t convert(char* p, size_t _size);
	char* getNewBuf() { return reinterpret_cast<char*>(m_pNewBuf); }

protected:
	encodingType	m_eEncoding;
	FILE*			m_pFile;
	utf16*			m_pBuf;
	ubyte*			m_pNewBuf;
	size_t			m_nBufSize;
	bool			m_bFirstWrite;
	bool			m_bIsBOM;
};
