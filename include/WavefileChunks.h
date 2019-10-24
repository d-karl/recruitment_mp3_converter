#ifndef WAVEFILE_WAVEFILECHUNKS_H
#define WAVEFILE_WAVEFILECHUNKS_H

#include <array>
#include <cstring>
#include <vector>

// for reference to these headers see
// https://stackoverflow.com/questions/13660777/c-reading-the-data-part-of-a-wav-file

/**
 * @namespace wavefile::cunks
 *
 * @brief Namespace that defines different types of chunk structs that can be
 * found in a wav file.
 */
namespace wavefile {
namespace chunks {
/**
 * @struct FourUnterminatedChars
 *
 * @brief A POD struct containing an array of 4 unterminated chars, that
 * provides save equivalency operators.
 *
 * By abstracting this, the common 4 char fields in a wav chunk can now be
 * transparently compared to normal strings, without thrad of UB.
 */
struct FourUnterminatedChars
{
    std::array<char, 4> chars;

    bool operator==(const std::string &literal) const
    {
        if (strncmp(chars.data(), literal.c_str(), 4) == 0)
            return true;
        else
            return false;
    }

    bool operator!=(const std::string &literal) const
    {
        return !operator==(literal);
    }
};

/**
 * @struct CommonHeader
 *
 * @brief A POD struct that represents the common header each wav file chunk
 * starts with.
 *
 * Since this is plain old data (POD), the memory layout is guaranteed and we
 * can simply copy from the file into this struct and the others to interpret
 * the fields. I don't think we need to align or pack here but it might be
 * required on some platforms. If that is the case, one of the asserts will
 * trigger.
 */
struct CommonHeader
{
    FourUnterminatedChars ChunkID;
    uint32_t              ChunkSize;
};
static_assert(sizeof(CommonHeader) == 8,
              "CommonHeader size does not fit spec.");

/**
 * @struct RiffHeader
 *
 * @brief A POD struct that represents the RIFF header.
 */
struct RiffHeader
{
    CommonHeader          Chunk_header;
    FourUnterminatedChars Format;
};
static_assert(sizeof(RiffHeader) == 12, "Riff Header size does not fit spec.");

/**
 * @struct FormatHeader

 * @brief A POD struct that represents the format chunk.
 */
struct FormatHeader
{
    CommonHeader Chunk_header;
    uint16_t     AudioFormat;
    uint16_t     NumChannels;
    uint32_t     SampleRate;
    uint32_t     ByteRate;
    uint16_t     BlockAlign;
    uint16_t     BitsPerSample;
};
static_assert(sizeof(FormatHeader) == 24, "Format header does not fit spec.");

/**
 * @struct DataChunk
 *
 * @brief A struct that represents a data chunk, with the Data field in
 * uninterpreted, binary format.
 */
struct DataChunk
{
    CommonHeader Chunk_header;

    std::vector<char> Data;
};
} // namespace chunks
} // namespace wavefile
#endif /* WAVEFILE_WAVEFILECHUNKS_H */
