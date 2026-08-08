#ifndef OBC_TOPCCSDS_ONBOARDSTATESNAPSHOTSOURCE_HPP
#define OBC_TOPCCSDS_ONBOARDSTATESNAPSHOTSOURCE_HPP

#include "OBC/Components/AdcsBridge/AdcsBridge.hpp"
#include "OBC/Components/BootManager/BootManager.hpp"
#include "OBC/Components/CommController/CommController.hpp"
#include "OBC/Components/CspBridge/CspBridge.hpp"
#include "OBC/Components/EpsBridge/EpsBridge.hpp"
#include "OBC/Components/GpsBridge/GpsBridge.hpp"
#include "OBC/Components/ModeManager/ModeManager.hpp"
#include "OBC/Components/OnboardStateData/OnboardStateData.hpp"
#include "OBC/Components/RadioController/RadioController.hpp"
#include "OBC/Components/StorageHealthBridge/StorageHealthBridge.hpp"
#include "OBC/Components/UartDriver/UartDriver.hpp"

namespace OBCApp {

class OnboardStateSnapshotSource final : public OBC::StateData::IStateSnapshotSource {
  public:
    OnboardStateSnapshotSource();

    void configure(const OBC::ModeManager* modeManager,
                   const OBC::EpsBridge* epsBridge,
                   const OBC::AdcsBridge* adcsBridge,
                   const OBC::GpsBridge* gpsBridge,
                   const OBC::StorageHealthBridge* storageHealthBridge,
                   const OBC::CommController* commController,
                   const OBC::CspBridge* cspBridge,
                   const OBC::RadioController* radioController,
                   const OBC::UartDriver* uartDriver,
                   const OBC::BootManager* bootManager);

    bool readStateSnapshot(OBC::StateData::StateSnapshot& snapshot) const override;

  private:
    const OBC::ModeManager* m_modeManager;
    const OBC::EpsBridge* m_epsBridge;
    const OBC::AdcsBridge* m_adcsBridge;
    const OBC::GpsBridge* m_gpsBridge;
    const OBC::StorageHealthBridge* m_storageHealthBridge;
    const OBC::CommController* m_commController;
    const OBC::CspBridge* m_cspBridge;
    const OBC::RadioController* m_radioController;
    const OBC::UartDriver* m_uartDriver;
    const OBC::BootManager* m_bootManager;
};

}  // namespace OBCApp

#endif
