/*  Xenoblade Engine Format Library
    Copyright(C) 2017-2022 Lukas Cone

    This program is free software : you can redistribute it and / or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.If not, see <https://www.gnu.org/licenses/>.
*/

#include "xenolib/sar.hpp"
#include "datas/endian.hpp"
#include "datas/except.hpp"

template <class C> void FByteswapper(SAR::Array<C> &item, bool = false) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(SAR::FileEntry &item, bool) {
  FByteswapper(item.data);
  FByteswapper(item.dataSize);
  FByteswapper(item.nameHash);
}

template <> void XN_EXTERN FByteswapper(SAR::Header &item, bool) {
  FByteswapper(item.data);
  FByteswapper(item.entries);
  FByteswapper(item.fileSize);
  FByteswapper(item.id);
  FByteswapper(item.unk0);
  FByteswapper(item.unk1);
  FByteswapper(item.version);
}

template <>
void XN_EXTERN ProcessClass(SAR::FileEntry &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();

  if (flags == ProcessFlag::EnsureBigEndian) {
    FByteswapper(item);
  }
  item.data.Fixup(flags.base);
}

template <> void XN_EXTERN ProcessClass(SAR::Header &item, ProcessFlags flags) {
  flags.NoProcessDataOut();

  if (flags == ProcessFlag::AutoDetectEndian) {
    flags -= ProcessFlag::AutoDetectEndian;

    if (item.id == SAR::ID_BIG) {
      flags += ProcessFlag::EnsureBigEndian;
    } else if (item.id == SAR::ID) {
      flags -= ProcessFlag::EnsureBigEndian;
    } else {
      throw es::InvalidHeaderError(item.id);
    }
  }
  flags.base = reinterpret_cast<char *>(&item);

  if (flags == ProcessFlag::EnsureBigEndian) {
    FByteswapper(item);
  }
  es::FixupPointers(flags.base, item.data, item.entries);

  for (auto &p : item.entries) {
    ProcessClass(p, flags);
  }
}

static const uint32 crcTable[0x100]{
    0x46D1E192, 0x66EDF9AA, 0x927FC9E5, 0xA53BAACC, 0x29B47658, 0x5A411A01,
    0x0E66D5BD, 0x0DD5B1DB, 0xCB38340E, 0x04D4EBB6, 0x98BC4F54, 0x36F20F2C,
    0x4A3047ED, 0x1EC1E0EB, 0x568C0C1F, 0x6A731432, 0x81367FC6, 0xE3E25237,
    0xE7F64884, 0x0FA59F64, 0x4F3109DE, 0xF02D61F5, 0x5DAEC03B, 0x7F740E83,
    0x056FF2D8, 0x2026CC0A, 0x7AC2112D, 0x82C55605, 0xB0911EF2, 0xA7B88E4C,
    0x89DCA282, 0x4B254D27, 0x7694A6D3, 0xD229EADD, 0x8E8F3738, 0x5BEE7A55,
    0x012EB6AB, 0x08DD28C8, 0xB5ABC274, 0xBC7931F0, 0xF2396ED5, 0xE4E43D97,
    0x943F4B7F, 0x85D0293D, 0xAED83A88, 0xC8F932FC, 0xC5496F20, 0xE9228173,
    0x9B465B7D, 0xFDA26680, 0x1DDEAB35, 0x0C4F25CB, 0x86E32FAF, 0xE59FA13A,
    0xE192E2C4, 0xF147DA1A, 0x67620A8D, 0x5C9A24C5, 0xFE6AFDE2, 0xACAD0250,
    0xD359730B, 0xF35203B3, 0x96A4B44D, 0xFBCACEA6, 0x41A165EC, 0xD71E53AC,
    0x835F39BF, 0x6B6BDE7E, 0xD07085BA, 0x79064E07, 0xEE5B20C3, 0x3B90BD65,
    0x5827AEF4, 0x4D12D31C, 0x9143496E, 0x6C485976, 0xD9552733, 0x220F6895,
    0xE69DEF19, 0xEB89CD70, 0xC9BB9644, 0x93EC7E0D, 0x2ACE3842, 0x2B6158DA,
    0x039E9178, 0xBB5367D7, 0x55682285, 0x4315D891, 0x19FD8906, 0x7D8D4448,
    0xB4168A03, 0x40B56A53, 0xAA3E69E0, 0xA25182FE, 0xAD34D16C, 0x720C4171,
    0x9DC3B961, 0x321DB563, 0x8B801B9E, 0xF5971893, 0x14CC1251, 0x8F4AE962,
    0xF65AFF1E, 0x13BD9DEE, 0x5E7C78C7, 0xDDB61731, 0x73832C15, 0xEFEBDD5B,
    0x1F959ACA, 0xE801FB22, 0xA89826CE, 0x30B7165D, 0x458A4077, 0x24FEC52A,
    0x849B065F, 0x3C6930CD, 0xA199A81D, 0xDB768F30, 0x2E45C64A, 0xFF2F0D94,
    0x4EA97917, 0x6F572ACF, 0x653A195C, 0x17A88C5A, 0x27E11FB5, 0x3F09C4C1,
    0x2F87E71B, 0xEA1493E4, 0xD4B3A55E, 0xBE6090BE, 0xAF6CD9D9, 0xDA58CA00,
    0x612B7034, 0x31711DAD, 0x6D7DB041, 0x8CA786B7, 0x09E8BF7A, 0xC3C4D7EA,
    0xA3CD77A8, 0x7700F608, 0xDF3DE559, 0x71C9353F, 0x9FD236FB, 0x1675D43E,
    0x390D9E9A, 0x21BA4C6B, 0xBD1371E8, 0x90338440, 0xD5F163D2, 0xB140FEF9,
    0x52F50B57, 0x3710CF67, 0x4C11A79C, 0xC6D6624E, 0x3DC7AFA9, 0x34A69969,
    0x70544A26, 0xF7D9EC98, 0x7C027496, 0x1BFB3BA3, 0xB3B1DC8F, 0x9A241039,
    0xF993F5A4, 0x15786B99, 0x26E704F7, 0x51503C04, 0x028BB3B8, 0xEDE5600C,
    0x9CB22B29, 0xB6FF339B, 0x7E771C43, 0xC71C05F1, 0x604CA924, 0x695EED60,
    0x688ED0BC, 0x3E0B232F, 0xF8A39C11, 0xBAE6E67C, 0xB8CF75E1, 0x970321A7,
    0x5328922B, 0xDEF3DF2E, 0x8D0443B0, 0x2885E3AE, 0x6435EED1, 0xCC375E81,
    0xA98495F6, 0xE0BFF114, 0xB2DA3E4F, 0xC01B5ADF, 0x507E0721, 0x6267A36A,
    0x181A6DF8, 0x7BAFF0C0, 0xFA6D6C13, 0x427250B2, 0xE2F742D6, 0xCD5CC723,
    0x2D218BE7, 0xB91FBBB1, 0x9EB946D0, 0x1C180810, 0xFC81D602, 0x0B9C3F52,
    0xC2EA456F, 0x1165B2C9, 0xABF4AD75, 0x0A56FC8C, 0x12E0F818, 0xCADBCBA1,
    0x2586BE56, 0x952C9B46, 0x07C6A43C, 0x78967DF3, 0x477B2E49, 0x2C5D7B6D,
    0x8A637272, 0x59ACBCB4, 0x74A0E447, 0xC1F8800F, 0x35C015DC, 0x230794C2,
    0x4405F328, 0xEC2ADBA5, 0xD832B845, 0x6E4ED287, 0x48E9F7A2, 0xA44BE89F,
    0x38CBB725, 0xBF6EF4E6, 0xDC0E83FA, 0x54238D12, 0xF4F0C1E3, 0xA60857FD,
    0xC43C64B9, 0x00C851EF, 0x33D75F36, 0x5FD39866, 0xD1EFA08A, 0xA0640089,
    0x877A978B, 0x99175D86, 0x57DFACBB, 0xCEB02DE9, 0xCF4D5C09, 0x3A8813D4,
    0xB7448816, 0x63FA5568, 0x06BE014B, 0xD642FA7B, 0x10AA7C90, 0x8082C88E,
    0x1AFCBA79, 0x7519549D, 0x490A87FF, 0x8820C3A0,
};

uint32 CRC32C(std::string_view str) {
  if (str.empty()) {
    return 0;
  }

  uint32 retVal = 0;

  for (uint8 c : str) {
    uint8 nibble = retVal ^ c;
    retVal = (retVal >> 8) ^ crcTable[nibble];
  }

  retVal = (retVal >> 8) ^
           *(uint32 *)((char *)crcTable + ((4 * str.size()) & 0x3FC));

  return retVal;
}

void SAR::FileEntry::RecalcHash() { nameHash = CRC32C(fileName); }
