/*******************************************************************************
 * Copyright (c) 2013, Art Clarke.  All rights reserved.
 *  
 * This file is part of Humble-Video.
 *
 * Humble-Video is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Humble-Video is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Humble-Video.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/
/*
 * MediaAudioTest.cpp
 *
 *  Created on: Jul 12, 2013
 *      Author: aclarke
 */

#include "MediaAudioTest.h"
#include <io/humble/ferry/RefPointer.h>
#include <io/humble/ferry/LoggerStack.h>

using namespace io::humble::video;
using namespace io::humble::ferry;

MediaAudioTest::MediaAudioTest() {
}

MediaAudioTest::~MediaAudioTest() {
}

void
MediaAudioTest::testCreation() {
  const int32_t numSamples = 1024;
  const int32_t sampleRate = 22050;
  const int32_t channels = 8;
  const AudioChannel::Layout layout = AudioChannel::CH_LAYOUT_7POINT1;
  const AudioFormat::Type format = AudioFormat::SAMPLE_FMT_S16P;

  // now let's test invalid methods.
  RefPointer<MediaAudio> audio;

  {
    LoggerStack stack;
    stack.setGlobalLevel(Logger::LEVEL_ERROR, false);

    audio = MediaAudio::make(-1, sampleRate, channels, layout, format);
    TS_ASSERT(!audio);
  }

  {
    LoggerStack stack;
    stack.setGlobalLevel(Logger::LEVEL_ERROR, false);

    audio = MediaAudio::make(numSamples, -1, channels, layout, format);
    TS_ASSERT(!audio);
  }

  {
    LoggerStack stack;
    stack.setGlobalLevel(Logger::LEVEL_ERROR, false);

    audio = MediaAudio::make(numSamples, sampleRate, -1, layout, format);
    TS_ASSERT(!audio);
  }

  {
    LoggerStack stack;
    stack.setGlobalLevel(Logger::LEVEL_ERROR, false);

    audio = MediaAudio::make(numSamples, sampleRate, channels + 1, layout,
        format);
    TS_ASSERT(!audio);
  }

  {
    LoggerStack stack;
    stack.setGlobalLevel(Logger::LEVEL_ERROR, false);

    audio = MediaAudio::make(numSamples, sampleRate, channels, layout,
        AudioFormat::SAMPLE_FMT_NONE);
    TS_ASSERT(!audio);
  }

  // And this should be valid
  audio = MediaAudio::make(numSamples, sampleRate, channels + 1,
      AudioChannel::CH_LAYOUT_UNKNOWN, format);
  TS_ASSERT(audio);

  audio = MediaAudio::make(numSamples, sampleRate, channels, layout, format);
  TS_ASSERT(audio);

  // now let's try getting the data
  RefPointer<IBuffer> buf;

  TS_ASSERT_EQUALS(numSamples, audio->getMaxNumSamples());
  TS_ASSERT_EQUALS(0, audio->getNumSamples());
  TS_ASSERT_EQUALS(channels, audio->getChannels());
  TS_ASSERT_EQUALS(channels, audio->getNumDataPlanes());
  TS_ASSERT_EQUALS(layout, audio->getChannelLayout());
  TS_ASSERT_EQUALS(sampleRate, audio->getSampleRate());
  TS_ASSERT_EQUALS(format, audio->getFormat());

  for (int i = 0; i < channels; i++) {
    buf = audio->getData(i);
    TS_ASSERT(buf);
    TS_ASSERT_EQUALS(audio->getDataPlaneSize(), buf->getBufferSize());
  }
  // now let's try packed audio
  audio = MediaAudio::make(numSamples, sampleRate, channels, layout,
      AudioFormat::getPackedSampleFormat(format));
  TS_ASSERT(audio);
  TS_ASSERT_EQUALS(AudioFormat::getPackedSampleFormat(format),
      audio->getFormat());
  TS_ASSERT_EQUALS(channels, audio->getChannels());
  TS_ASSERT_EQUALS(1, audio->getNumDataPlanes());

  buf = audio->getData(0);
  TS_ASSERT(buf);
  for (int i = 1; i < channels; i++) {
    LoggerStack stack;
    stack.setGlobalLevel(Logger::LEVEL_ERROR, false);
    buf = audio->getData(i);
    TS_ASSERT(!buf);
  }

}

void
MediaAudioTest::testCreationFromBufferPacked() {
  const int32_t numSamples = 155; // I choose an odd number because HV will align up to 32
  const int32_t sampleRate = 22050;
  const int32_t channels = 8;
  const AudioChannel::Layout layout = AudioChannel::CH_LAYOUT_7POINT1;
  const AudioFormat::Type format = AudioFormat::SAMPLE_FMT_DBL;

  int32_t bufSize = AudioFormat::getBufferSizeNeeded(numSamples, channels, format);
  RefPointer<IBuffer> src = IBuffer::make(0, bufSize);
  double* srcData = (double*)src->getBytes(0, bufSize);

  // now, let's go nuts!
  for(size_t i = 0; i < bufSize/sizeof(double); i++)
  {
    srcData[i] = i;
  }

  RefPointer<MediaAudio> audio;

  {
    LoggerStack stack;
    stack.setGlobalLevel(Logger::LEVEL_ERROR, false);

    audio = MediaAudio::make(0, numSamples, sampleRate, channels, layout,
        format);
    TS_ASSERT(!audio);
  }
  audio = MediaAudio::make(src.value(), numSamples, sampleRate, channels, layout,
      format);
  TS_ASSERT(audio);

  TS_ASSERT_EQUALS(1, audio->getNumDataPlanes());
  RefPointer<IBuffer> dst = audio->getData(0);
  TS_ASSERT(dst);
  double* dstData = (double*)dst->getBytes(0, dst->getBufferSize());

  // the test in packed data is simple -- the buffers should be equal!
  TS_ASSERT_EQUALS(src->getBufferSize(), dst->getBufferSize());
  TS_ASSERT_EQUALS(srcData, dstData);

}

void
MediaAudioTest::testCreationFromBufferPlanar() {
  const int32_t numSamples = 155; // I choose an odd number because HV will align up to 32
  const int32_t sampleRate = 22050;
  const int32_t channels = 15; // choose a large # of channels to make sure we expand the Frame
  const AudioChannel::Layout layout = AudioChannel::CH_LAYOUT_UNKNOWN;
  const AudioFormat::Type format = AudioFormat::SAMPLE_FMT_DBLP;

  int32_t bufSize = AudioFormat::getBufferSizeNeeded(numSamples, channels, format);
  // test that there is rounding up
  int32_t minSize = AudioFormat::getBytesPerSample(format)*numSamples*channels;
  TS_ASSERT_LESS_THAN(minSize, bufSize);

  RefPointer<IBuffer> src = IBuffer::make(0, bufSize);
  double* srcData = (double*)src->getBytes(0, bufSize);

  // now, let's go nuts!
  for(size_t i = 0; i < bufSize/sizeof(double); i++)
  {
    srcData[i] = i;
  }

  RefPointer<MediaAudio> audio;

  {
    LoggerStack stack;
    stack.setGlobalLevel(Logger::LEVEL_ERROR, false);

    audio = MediaAudio::make(0, numSamples, sampleRate, channels, layout,
        format);
    TS_ASSERT(!audio);
  }
  audio = MediaAudio::make(src.value(), numSamples, sampleRate, channels, layout,
      format);
  TS_ASSERT(audio);

  TS_ASSERT_EQUALS(channels, audio->getNumDataPlanes());

  // sigh; time to test each plane.
  for(int i = 0; i < channels; i++) {
    RefPointer<IBuffer> dst = audio->getData(i);
    TS_ASSERT(dst);
    double* dstData = (double*)dst->getBytes(0, dst->getBufferSize());

    // the values should be monotonically increasing given how we set them.
    double last = dstData[0];
    for(size_t j=1; j < dst->getBufferSize()/sizeof(double); j++)
    {
      TS_ASSERT_DELTA(last+1, dstData[j], 0.001);
      last = dstData[j];
    }
  }
}
