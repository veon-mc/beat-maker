#pragma once

/**
 * @class Mixer
 * @brief Mixes multiple audio sources into a single output buffer.
 *
 * The mixer performs additive mixing: each call adds a scaled source
 * buffer to the destination buffer. To mix N tracks, the destination
 * must be cleared (set to zero) before the first mix() call, then
 * mix() is called once per track.
 */
class Mixer
{
public:
    /**
     * @brief Adds a source buffer into a destination buffer with a volume scale.
     *
     * Both buffers are interleaved stereo: 2 floats per frame (L, R).
     * The operation is destination[i] += source[i] * volume for each sample.
     *
     * @param source The input buffer to add (must contain 2 * numFrames floats).
     * @param destination The output buffer to add into (must contain 2 * numFrames floats).
     * @param numFrames The number of stereo frames to process.
     * @param volume The volume scaling factor applied to the source (0.0 = silent, 1.0 = unchanged).
     */
    void mix(const float* source, float* destination, int numFrames, float volume);
};