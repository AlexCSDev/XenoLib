/*  TEX2DDS
    Copyright(C) 2022 Lukas Cone

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

#include "datas/app_context.hpp"
#include "datas/binreader_stream.hpp"
#include "datas/binwritter_stream.hpp"
#include "datas/master_printer.hpp"
#include "dds.hpp"
#include "project.h"
#include "xenolib/xbc1.hpp"

std::string_view filters[]{
    ".catex$", ".witex$", ".wismt$", ".calut$", ".witx$", ".pcsmt$",
};

static AppInfo_s appInfo{
    .filteredLoad = true,
    .header =
        TEX2DDS_DESC " v" TEX2DDS_VERSION ", " TEX2DDS_COPYRIGHT "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

void AppProcessFile(AppContext *ctx) {
  std::string texData;
  std::string hiData;

  if (ctx->workingFile.GetExtension() == ".wismt") {
    auto folder = ctx->workingFile.GetFolder();

    if (!folder.ends_with("tex/nx/m/")) {
      printwarning("Supplied wismt file folder is not tex/nx/m, skipping");
      return;
    }

    uint32 id;
    if (ctx->GetType(id); id != CompileFourCC("xbc1")) {
      printinfo("File is stream, not texture, skipping.");
      return;
    }

    std::string buffer = ctx->GetBuffer();
    texData = DecompressXBC1(buffer.data());

    auto hiStream = ctx->RequestFile(
        std::string(folder).replace(folder.size() - 2, 1, "h") +
        std::string(ctx->workingFile.GetFilenameExt()));
    BinReaderRef hird(*hiStream.Get());
    hird.ReadContainer(buffer, hird.GetSize());
    hiData = DecompressXBC1(buffer.data());
  } else if (ctx->workingFile.GetExtension() == ".pcsmt") {
    uint32 id;
    if (ctx->GetType(id); id != CompileFourCC("xbc1")) {
      printinfo("File is stream, not texture, skipping.");
      return;
    }

    std::string buffer = ctx->GetBuffer();
    texData = DecompressXBC1(buffer.data());

    BinWritterRef wr(ctx->NewFile(ctx->workingFile.ChangeExtension(".dds")));

    wr.WriteContainer(texData);
    return;
  } else {
    texData = ctx->GetBuffer();
  }

  if (auto hdr = MTXT::Mount(texData); hdr->id == MTXT::ID) {
    FByteswapper(*const_cast<MTXT::Header *>(hdr));
    BinWritterRef wr(ctx->NewFile(ctx->workingFile.ChangeExtension(".dds")));

    auto dds = ToDDS(hdr);
    wr.Write(dds);
    std::string outBuffer;
    DDS::Mips dummy;
    outBuffer.resize(dds.ComputeBufferSize(dummy));
    MTXT::DecodeMipmap(*hdr, texData.data(), outBuffer.data());
    wr.WriteContainer(outBuffer);
  } else if (auto hdr = LBIM::Mount(texData); hdr->id == LBIM::ID) {
    BinWritterRef wr(ctx->NewFile(ctx->workingFile.ChangeExtension(".dds")));

    if (!hiData.empty()) {
      auto mutHdr = const_cast<LBIM::Header *>(hdr);
      mutHdr->width *= 2;
      mutHdr->height *= 2;
    }

    auto dds = ToDDS(hdr);
    wr.Write(dds);
    std::string outBuffer;
    DDS::Mips dummy;
    outBuffer.resize(dds.ComputeBufferSize(dummy));
    LBIM::DecodeMipmap(*hdr, hiData.empty() ? texData.data() : hiData.data(),
                       outBuffer.data());
    wr.WriteContainer(outBuffer);
  } else {
    throw std::runtime_error("Not valid texture");
  }
}
