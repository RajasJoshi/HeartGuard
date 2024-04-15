#include "ecg.hpp"
#include <gtest/gtest.h>


TEST(ECGTest, BufferInitialisation) {
  ECG ecg;
  EXPECT_EQ(ecg.BUFFER_SIZE, 860 * 4); // 4 seconds of data at 860 Hz
}

TEST(ECG, DetectedPeaksInitialisation) {
  ECG ecg;
  EXPECT_TRUE(ecg.detected_peaks.empty());
}

TEST(ECG, RRIntervalsInitialisation) {
  ECG ecg;
  EXPECT_TRUE(ecg.RR_intervals.empty());
}

TEST(ECG, HRVInitialisation) {
  ECG ecg;
  EXPECT_TRUE(ecg.HRV.empty());
}

TEST(ECG, CircularBufferInitialisation) {
  ECG ecg;
  for (int i = 0; i < ECG::BUFFER_SIZE; i++) {
    EXPECT_EQ(ecg.circularBuffer[i], 0.0f);
  }
}


TEST(ECGTest, RecalculateMean) {
  ECG ecg;
  for (int i = 0; i < ECG::BUFFER_SIZE; i++) {
    ecg.circularBuffer[i] = 100.0f; 
  }
  ecg.recalculate_mean();
  EXPECT_EQ(ecg.mean, 100.0f);
}

TEST(ECGTest, RecalculateStdev) {
  ECG ecg;
  for (int i = 0; i < ECG::BUFFER_SIZE; i++) {
    ecg.circularBuffer[i] = 100.0f; 
  }
  ecg.recalculate_mean();
  ecg.recalculate_stdev();
  EXPECT_EQ(ecg.stdev, 0.0f);
}

// ... Similar tests for recalculate_stdev() and recalculate_threshold()
