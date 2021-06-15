/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
*/

// includes
#include <AK/Atomic.h>
#include <AK/Debug.h>
#include <AK/String.h>
#include <LibAudio/Buffer.h>

namespace Audio {

u16 pcm_bits_per_sample(PcmSampleFormat format)
{
    switch (format) {
    case Uint8:
        return 8;
    case Int16:
        return 16;
    case Int24:
        return 24;
    case Float32:
        return 32;
    case Float64:
        return 64;
    default:
        VERIFY_NOT_REACHED();
    }
}

String sample_format_name(PcmSampleFormat format)
{
    bool is_float = format == Float32 || format == Float64;
    return String::formatted("PCM {}bit {}", pcm_bits_per_sample(format), is_float ? "Float" : "LE");
}

i32 Buffer::allocate_id()
{
    static Atomic<i32> next_id;
    return next_id++;
}

template<typename SampleReader>
static void read_samples_from_stream(InputMemoryStream& stream, SampleReader read_sample, Vector<Frame>& samples, ResampleHelper& resampler, int num_channels)
{
    double norm_l = 0;
    double norm_r = 0;

    switch (num_channels) {
    case 1:
        for (;;) {
            while (resampler.read_sample(norm_l, norm_r)) {
                samples.append(Frame(norm_l));
            }
            norm_l = read_sample(stream);

            if (stream.handle_any_error()) {
                break;
            }
            resampler.process_sample(norm_l, norm_r);
        }
        break;
    case 2:
        for (;;) {
            while (resampler.read_sample(norm_l, norm_r)) {
                samples.append(Frame(norm_l, norm_r));
            }
            norm_l = read_sample(stream);
            norm_r = read_sample(stream);

            if (stream.handle_any_error()) {
                break;
            }
            resampler.process_sample(norm_l, norm_r);
        }
        break;
    default:
        VERIFY_NOT_REACHED();
    }
}

static double read_float_sample_64(InputMemoryStream& stream)
{
    LittleEndian<double> sample;
    stream >> sample;
    return double(sample);
}

static double read_float_sample_32(InputMemoryStream& stream)
{
    LittleEndian<float> sample;
    stream >> sample;
    return double(sample);
}

static double read_norm_sample_24(InputMemoryStream& stream)
{
    u8 byte = 0;
    stream >> byte;
    u32 sample1 = byte;
    stream >> byte;
    u32 sample2 = byte;
    stream >> byte;
    u32 sample3 = byte;

    i32 value = 0;
    value = sample1 << 8;
    value |= (sample2 << 16);
    value |= (sample3 << 24);
    return double(value) / NumericLimits<i32>::max();
}

static double read_norm_sample_16(InputMemoryStream& stream)
{
    LittleEndian<i16> sample;
    stream >> sample;
    return double(sample) / NumericLimits<i16>::max();
}

static double read_norm_sample_8(InputMemoryStream& stream)
{
    u8 sample = 0;
    stream >> sample;
    return double(sample) / NumericLimits<u8>::max();
}

RefPtr<Buffer> Buffer::from_pcm_data(ReadonlyBytes data, ResampleHelper& resampler, int num_channels, PcmSampleFormat sample_format)
{
    InputMemoryStream stream { data };
    return from_pcm_stream(stream, resampler, num_channels, sample_format, data.size() / (pcm_bits_per_sample(sample_format) / 8));
}

RefPtr<Buffer> Buffer::from_pcm_stream(InputMemoryStream& stream, ResampleHelper& resampler, int num_channels, PcmSampleFormat sample_format, int num_samples)
{
    Vector<Frame> fdata;
    fdata.ensure_capacity(num_samples);

    switch (sample_format) {
    case PcmSampleFormat::Uint8:
        read_samples_from_stream(stream, read_norm_sample_8, fdata, resampler, num_channels);
        break;
    case PcmSampleFormat::Int16:
        read_samples_from_stream(stream, read_norm_sample_16, fdata, resampler, num_channels);
        break;
    case PcmSampleFormat::Int24:
        read_samples_from_stream(stream, read_norm_sample_24, fdata, resampler, num_channels);
        break;
    case PcmSampleFormat::Float32:
        read_samples_from_stream(stream, read_float_sample_32, fdata, resampler, num_channels);
        break;
    case PcmSampleFormat::Float64:
        read_samples_from_stream(stream, read_float_sample_64, fdata, resampler, num_channels);
        break;
    default:
        VERIFY_NOT_REACHED();
    }

    VERIFY(!stream.handle_any_error());

    return Buffer::create_with_samples(move(fdata));
}

}
