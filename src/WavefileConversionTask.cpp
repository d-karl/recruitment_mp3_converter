#include "WavefileConversionTask.h"

#include "LameEncodingTask.h"
#include "WavefileChunks.h"

#include <algorithm>
#include <atomic>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>


// static assert type sizes

namespace {
std::atomic<size_t> static_num_task {};

/**
 * @brief Split the samples from raw data into individual buffers based on
 * num_channels.
 *
 * @param to_file File name to Encode into.
 * @param data The uninterpreted wave data chunk.
 * @param num_channels The number of channels according to format header.
 * @param sample_rate The sample rate according to format header.
 */
template<typename R>
bool convert_from_raw(const std::filesystem::path &      to_file,
                      const wavefile::chunks::DataChunk &data,
                      unsigned long                      num_samples,
                      int                                num_channels,
                      int                                sample_rate)
{
    mp3_converter::LameEncodingTask<R> lame_encoding_task(to_file);

    const R *interpreted_array_ptr =
        reinterpret_cast<const R *>(data.Data.data());

    if (num_channels == 1)
    {
        return lame_encoding_task.Encode(num_samples,
                                         sample_rate,
                                         num_channels,
                                         interpreted_array_ptr,
                                         interpreted_array_ptr);
    }
    else if (num_channels == 2)
    {
        std::vector<R> l_buffer;
        std::vector<R> r_buffer;
        l_buffer.reserve(num_samples);
        r_buffer.reserve(num_samples);
        for (size_t i = 0; i < num_samples * 2; ++i)
        {
            if (i % 2 == 0)
            {
                l_buffer.push_back(interpreted_array_ptr[i]);
            }
            else
            {
                r_buffer.push_back(interpreted_array_ptr[i]);
            }
        }
        return lame_encoding_task.Encode(num_samples,
                                         sample_rate,
                                         num_channels,
                                         l_buffer.data(),
                                         r_buffer.data());
    }
    return false;
}
} // namespace

namespace mp3_converter {

WavefileConversionTask::WavefileConversionTask(
    const std::filesystem::path &in_file)
    : m_task_num(static_num_task++)
    , m_wav_file_in(in_file)
{
}

bool WavefileConversionTask::run()
{
    std::cout << "Thread " + std::to_string(m_task_num)
                     + ": Starting conversion.\n";

    using std::ifstream;
    using namespace wavefile::chunks;

    ifstream i_stream;
    i_stream.open(m_wav_file_in.string(), std::ios_base::binary);

    auto file_out = m_wav_file_in;
    file_out.replace_extension("mp3");

    RiffHeader header;
    i_stream.read(reinterpret_cast<char *>(&header), sizeof(header));

    // use FourUnterminatedChars comparison operators to check for equivalency
    // in a safe manner (ChunkID is unterminated string).
    if (header.Chunk_header.ChunkID != "RIFF")
    {
        throw wave_format_error("File does not contain a RIFF file header.");
    }
    if (header.Format != "WAVE")
    {
        throw wave_format_error("File does not contain a WAVE file header.");
    }
    // there can now be a number of unknown chunks until we find "fmt", then
    // "data"

    // local lambda to check chunks until the one with the given identifier has
    // been read in
    auto find_chunk =
        [&](const std::string &identifier) -> std::optional<CommonHeader> {
        CommonHeader potential_chunk;

        while (i_stream.good())
        {
            i_stream.read(reinterpret_cast<char *>(&potential_chunk),
                          sizeof(CommonHeader));
            if (potential_chunk.ChunkID == identifier)
            {
                return potential_chunk;
            }
            else
            {
                // skip over the unknown  chunk
                size_t to_skip = potential_chunk.ChunkSize;
                i_stream.seekg(to_skip, std::ios_base::cur);
            }
        }
        return std::nullopt;
    };

    auto maybe_format_start = find_chunk("fmt ");
    if (!maybe_format_start)
        throw wave_format_error("Did not find fmt chunk in file.");

    FormatHeader format_header {maybe_format_start->ChunkID,
                                maybe_format_start->ChunkSize};

    i_stream.read(reinterpret_cast<char *>(&format_header.AudioFormat),
                  format_header.Chunk_header.ChunkSize);

    const uint16_t num_channels = format_header.NumChannels;
    if (num_channels > 2)
        throw wave_format_error(
            "Found more than two channels in wave file, unsuported");

    const uint16_t sample_rate = format_header.SampleRate;

    auto maybe_data_start = find_chunk("data");
    if (!maybe_data_start)
        throw wave_format_error("Did not find data chunk in file.");

    // determine the data type in the data chunk
    bool         conversion_success = false;
    const size_t data_size          = maybe_data_start->ChunkSize;
    // num frames = how many blocks of #num_channels are in file
    const unsigned long num_frames = data_size / format_header.BlockAlign;

    DataChunk data;
    data.Chunk_header.ChunkID   = maybe_data_start->ChunkID;
    data.Chunk_header.ChunkSize = maybe_data_start->ChunkSize;
    data.Data.resize(data_size);
    i_stream.read(data.Data.data(), data_size);

    if (format_header.AudioFormat == 1) // PCM format
    {
        // integer format
        if (format_header.BlockAlign / num_channels == sizeof(short))
        {
            conversion_success = convert_from_raw<short>(
                file_out, data, num_frames, num_channels, sample_rate);
        }
        else if (format_header.BlockAlign / num_channels == sizeof(int))
        {
            conversion_success = convert_from_raw<int>(
                file_out, data, num_frames, num_channels, sample_rate);
        }
        else if (format_header.BlockAlign / num_channels == sizeof(long))
        {
            conversion_success = convert_from_raw<long>(
                file_out, data, num_frames, num_channels, sample_rate);
        }
        else
        {
            throw wave_format_error(
                "Found integer samples that don't align to: this platforms "
                "length of short, int, long. Unsupported.");
        }
    }
    else if (format_header.AudioFormat == 3) // IEEE float
    {
        if (format_header.BlockAlign / num_channels == sizeof(float))
        {
            conversion_success = convert_from_raw<float>(
                file_out, data, num_frames, num_channels, sample_rate);
        }
        if (format_header.BlockAlign / num_channels == sizeof(double))
        {
            conversion_success = convert_from_raw<double>(
                file_out, data, num_frames, num_channels, sample_rate);
        }
    }
    else
    {
        throw wave_format_error(
            "Wave format is neither PCM nor IEEE_FLOAT. Unsupported.");
    }

    return conversion_success;
}

} // namespace mp3_converter
