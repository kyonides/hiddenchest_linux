#ifndef AUDIODATA_H
#define AUDIODATA_H

#include <string>

struct AudioData
{
  std::string type;
  std::string ext;
  int sample_rate;
  int samples;
  double seconds;
};

#endif