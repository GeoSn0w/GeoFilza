/*
 * OSSerializeBinary.h
 * Brandon Azad
 */
#ifndef VOUCHER_SWAP__OSSERIALIZEBINARY_H_
#define VOUCHER_SWAP__OSSERIALIZEBINARY_H_

enum
{
	kOSSerializeDictionary   = 0x01000000U,
	kOSSerializeArray        = 0x02000000U,
	kOSSerializeSet          = 0x03000000U,
	kOSSerializeNumber       = 0x04000000U,
	kOSSerializeSymbol       = 0x08000000U,
	kOSSerializeString       = 0x09000000U,
	kOSSerializeData         = 0x0a000000U,
	kOSSerializeBoolean      = 0x0b000000U,
	kOSSerializeObject       = 0x0c000000U,
	kOSSerializeTypeMask     = 0x7F000000U,
	kOSSerializeDataMask     = 0x00FFFFFFU,
	kOSSerializeEndCollecton = 0x80000000U,
};

#define kOSSerializeBinarySignature 0x000000D3U

#endif
