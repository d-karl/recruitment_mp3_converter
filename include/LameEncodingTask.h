#ifndef LAME_ENCODING_LAMEENCODINGTASK_H
#define LAME_ENCODING_LAMEENCODINGTASK_H

#include "lame.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace mp3_converter {

/**
 * @struct lame_encoding_error
 *
 * @brief An error class indicating that lame encoding failed.
 */
struct lame_encoding_error : std::runtime_error
{
    // inherit c'tors
    using std::runtime_error::runtime_error;
};

/**
 * @struct unsupported_format
 *
 * @brief A templated struct to allow sensible compiler warnings for unsupported
 * types T of the following templated class.
 */
template<typename f>
struct unsupported_format
{
    constexpr static bool value()
    {
        return false;
    }
};

/**
 * @class LameEncodingTask
 *
 * @brief A templated task to convert differently formatted wav buffers to mp3
 * via the lame encoder.
 *
 * @tparam T The type of data inside the buffers handed to this class for
 * Encode. Must be signed and fixed width. The compiler will assert
 * compatibility at compile-time.
 */
template<typename T>
class LameEncodingTask
{
  public:
    static_assert(std::is_signed_v<T>, "Data type is unsigned, unuspported.");
    static_assert(std::is_arithmetic_v<T>,
                  "Data is not arithmetic type, that is float or integer.");

    /**
     * @brief Construct a LameEncodingTask that outputs to the given out_file.
     *
     * @param out_file The file to output to.
     */
    LameEncodingTask(const std::filesystem::path &out_file)
    {
        out_stream.open(out_file.string(), std::ios_base::binary);
    }

    /**
     * @brief Encode the given raw data passed in buffers according to the
     * parameters listed.
     *
     * In case of mono data (num_channels ==1), it is expected that buffer_l and
     * buffer_r point to the same data.
     *
     * @param num_sample The number of samples, that is the length of the passed
     * buffers.
     * @param sample_rate The sample rate of the raw wav data.
     * @param num_channels The number of channels, limited to 1 and 2 by lame.
     * @param buffer_l Ptr to buffer of raw data on left channel.
     * @param buffer_r Ptr to buffer of raw data on right channel.
     */
    bool Encode(unsigned long num_samples,
                int           sample_rate,
                int           num_channels,
                const T *     buffer_l,
                const T *     buffer_r)
    {
        auto *lame_flags = lame_init();
        if (lame_flags == nullptr)
            throw lame_encoding_error(
                "Error creating lame flags struct, malloc failed!");

        lame_set_num_samples(lame_flags, num_samples);
        lame_set_in_samplerate(lame_flags, sample_rate);
        lame_set_num_channels(lame_flags, num_channels);

        // redirect C callbacks that signal lame errors into an error we can
        // forward to callers.
        auto lame_error_forwarder = [](const char *format, va_list ap) {
            size_t            size = vsnprintf(nullptr, 0, format, ap);
            std::vector<char> err;
            err.resize(size + 1);
            vsprintf(err.data(), format, ap);

            throw lame_encoding_error(err.data());
        };

        lame_set_errorf(lame_flags, lame_error_forwarder);

        lame_init_params(lame_flags);

        // create an output array.
        std::vector<unsigned char> out_mp3_buf;
        // upper bound for buffer size taken from description in lame.h
        const auto out_buf_size =
            static_cast<unsigned long>((1.25 * num_samples) + 7200);
        out_mp3_buf.resize(out_buf_size);
        int bytes_encoded {};

        // use constexpr to decide which lame_encode_buffer function is used at
        // compile time depending on the type T.
        if constexpr (std::is_integral_v<T>)
        {
            // lame only cares about raw bytes, that means we choose the
            // appropriate function to encode based on the types length, and
            // static cast in case it has the same size as one of the accepted
            // ones, but is a different type. (for instance if sizeof(long long)
            // == sizeof(long))
            if constexpr (sizeof(T) == sizeof(short))
            {
                bytes_encoded = lame_encode_buffer(
                    lame_flags,
                    reinterpret_cast<const short *>(buffer_l),
                    reinterpret_cast<const short *>(buffer_r),
                    num_samples,
                    out_mp3_buf.data(),
                    out_buf_size);
            }
            else if constexpr (sizeof(T) == sizeof(int))
            {
                bytes_encoded = lame_encode_buffer_int(
                    lame_flags,
                    reinterpret_cast<const int *>(buffer_l),
                    reinterpret_cast<const int *>(buffer_r),
                    num_samples,
                    out_mp3_buf.data(),
                    out_buf_size);
            }
            else if constexpr (sizeof(T) == sizeof(long))
            {
                bytes_encoded = lame_encode_buffer_long2(
                    lame_flags,
                    reinterpret_cast<const long *>(buffer_l),
                    reinterpret_cast<const long *>(buffer_r),
                    num_samples,
                    out_mp3_buf.data(),
                    out_buf_size);
            }
            else
            {
                // this gives an understandable compile-time error if a type
                // with unsupported width is used.
                static_assert(unsupported_format<T>::value(),
                              "Invalid integral type for LameEncodingTask, "
                              "lame only supports short, int, long.");
            }
        } // end if integral_type
        else if constexpr (std::is_floating_point_v<T>)
        {
            if constexpr (sizeof(T) == sizeof(float))
            {
                bytes_encoded =
                    lame_encode_buffer_ieee_float(lame_flags,
                                                  buffer_l,
                                                  buffer_r,
                                                  num_samples,
                                                  out_mp3_buf.data(),
                                                  out_buf_size);
            }
            else if constexpr (sizeof(T) == sizeof(double))
            {
                bytes_encoded =
                    lame_encode_buffer_ieee_double(lame_flags,
                                                   buffer_l,
                                                   buffer_r,
                                                   num_samples,
                                                   out_mp3_buf.data(),
                                                   out_buf_size);
            }
            else
            {
                // this gives an understandable compile-time error if a type
                // with unsupported width is used.
                static_assert(
                    unsupported_format<T>::value(),
                    "Invalid floating type for LameEncodingTask, lame only "
                    "supports float, double.");
            }
        }
        bytes_encoded += lame_encode_flush(
            lame_flags, out_mp3_buf.data(), out_buf_size - bytes_encoded);

        lame_close(lame_flags);

        out_stream.write(reinterpret_cast<const char *>(out_mp3_buf.data()),
                         bytes_encoded);

        out_stream.close();

        return true;
    }

  private:
    std::ofstream out_stream {};
};

} // namespace mp3_converter
#endif /* LAME_ENCODING_LAMEENCODINGTASK_H */
