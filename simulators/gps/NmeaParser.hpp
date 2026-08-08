#ifndef OBC_SIMULATORS_GPS_NMEAPARSER_HPP
#define OBC_SIMULATORS_GPS_NMEAPARSER_HPP

#include <string>

#include "simulators/gps/GpsTypes.hpp"

namespace OBC {
namespace GPS {

ParseStatus parseNmeaSentence(const std::string& sentence, SentenceUpdate& update);

}  // namespace GPS
}  // namespace OBC

#endif
