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

} // namespace

std::shared_ptr<Ship::IResource>
ResourceFactoryBinaryRelocFileV0::ReadResource(std::shared_ptr<Ship::File> file,
                                                std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    auto relocFile = std::make_shared<RelocFile>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    ReadCommonPrefix(*reader, *relocFile);
    relocFile->ProcessingFlags = 0;     // v0: torch did no transforms
    ReadDataBlock(*reader, *relocFile);

    return relocFile;
}

std::shared_ptr<Ship::IResource>
ResourceFactoryBinaryRelocFileV1::ReadResource(std::shared_ptr<Ship::File> file,
                                                std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    auto relocFile = std::make_shared<RelocFile>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    ReadCommonPrefix(*reader, *relocFile);
    relocFile->ProcessingFlags = reader->ReadUInt32();
    ReadDataBlock(*reader, *relocFile);

    return relocFile;
}
