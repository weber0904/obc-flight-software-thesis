#ifndef OBC_COMPONENTS_GROUNDLINKHEALTHPROVIDER_HPP
#define OBC_COMPONENTS_GROUNDLINKHEALTHPROVIDER_HPP

#include <mutex>

#include "OBC/Components/GroundLinkDriver/GroundLinkDriver.hpp"
#include "OBC/Components/GroundLinkHealthProvider/GroundLinkHealthProviderComponentAc.hpp"
#include "OBC/Components/GroundLinkHealthProvider/GroundLinkHealthRuntime.hpp"

namespace OBC {

class GroundLinkHealthProvider final : public GroundLinkHealthProviderComponentBase {
  public:
    explicit GroundLinkHealthProvider(const char* const compName);

    ~GroundLinkHealthProvider() override;

    void configureRuntime(GroundLinkDriver* sbandGroundLinkDriver, GroundLinkDriver* uhfGroundLinkDriver);

    void tickForTest();

    OBC::CommLinkHealthView getHealthForRuntime(OBC::CommBand band) const;

    void markTelemetryDirtyForRuntime(OBC::CommBand band);

  private:
    struct TrackedLinkState {
        OBC::CommLinkHealthView view = {};
        OBC::COMM::GroundLinkObservationState lastObservation = {};
        bool initialized = false;
        OBC::CommLinkHealthView lastPublishedView = {};
        bool havePublishedTelemetry = false;
    };

  private:
    void schedIn_handler(FwIndexType portNum, U32 context) override;

    void updateTrackedState_(OBC::CommBand band, GroundLinkDriver* driver, TrackedLinkState& trackedState);

    void publishTelemetry_();

  private:
    mutable std::mutex m_mutex;
    GroundLinkDriver* m_sbandGroundLinkDriver;
    GroundLinkDriver* m_uhfGroundLinkDriver;
    TrackedLinkState m_sbandState;
    TrackedLinkState m_uhfState;
};

}  // namespace OBC

#endif
