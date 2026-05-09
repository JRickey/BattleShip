#include "RelocFileFactory.h"
#include "RelocFile.h"

namespace {

void ReadCommonPrefix(Ship::BinaryReader& reader, RelocFile& out) {
    out.FileId = reader.ReadUInt32();
    out.RelocInternOffset = reader.ReadUInt16();
    out.RelocExternOffset = reader.ReadUInt16();

    uint32_t numExternIds = reader.ReadUInt32();
    out.ExternFileIds.reserve(numExternIds);
    for (uint32_t i = 0; i < numExternIds; i++) {
        out.ExternFileIds.push_back(reader.ReadUInt16());
    }
}

void ReadDataBlock(Ship::BinaryReader& reader, RelocFile& out) {
    uint32_t dataSize = reader.ReadUInt32();
    out.Data.resize(dataSize);
    reader.Read(reinterpret_cast<char*>(out.Data.data()), static_cast<int32_t>(dataSize));
}

void ReadChainList(Ship::BinaryReader& reader, std::vector<RelocChainEntry>& out, uint16_t depFileIdSentinel) {
    uint32_t count = reader.ReadUInt32();
    out.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        RelocChainEntry e;
        e.SlotByteOffset = reader.ReadUInt32();
        e.TargetByteOffset = reader.ReadUInt32();
        e.DepFileId = depFileIdSentinel;  // intern: 0xFFFF; extern: filled below
        out.push_back(e);
    }
}

void ReadSubResourceList(Ship::BinaryReader& reader, std::vector<RelocSubResource>& out) {
    uint32_t count = reader.ReadUInt32();
    out.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        RelocSubResource sr;
        sr.Hash       = reader.ReadUInt64();
        sr.ByteOffset = reader.ReadUInt32();
        sr.Size       = reader.ReadUInt32();
        sr.Kind       = static_cast<SubResKind>(reader.ReadUByte());
        (void)reader.ReadUByte();   // pad[0]
        (void)reader.ReadUByte();   // pad[1]
        (void)reader.ReadUByte();   // pad[2]
        out.push_back(sr);
    }
}

} // namespace

std::shared_ptr<Ship::IResource>
ResourceFactoryBinaryRelocFileV3::ReadResource(std::shared_ptr<Ship::File> file,
                                                std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    auto relocFile = std::make_shared<RelocFile>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    ReadCommonPrefix(*reader, *relocFile);
    relocFile->ProcessingFlags = reader->ReadUInt32();
    ReadChainList(*reader, relocFile->InternChain, /*depFileIdSentinel=*/0xFFFF);
    ReadChainList(*reader, relocFile->ExternChain, /*depFileIdSentinel=*/0xFFFF);
    if (relocFile->ExternChain.size() == relocFile->ExternFileIds.size()) {
        for (size_t i = 0; i < relocFile->ExternChain.size(); i++) {
            relocFile->ExternChain[i].DepFileId = relocFile->ExternFileIds[i];
        }
    }
    ReadSubResourceList(*reader, relocFile->SubResourceHashes);
    ReadDataBlock(*reader, *relocFile);

    return relocFile;
}
