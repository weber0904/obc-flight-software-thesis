#ifndef OBC_COMPONENTS_UHFCCSDSVCIDADAPTER_TESTER_HPP
#define OBC_COMPONENTS_UHFCCSDSVCIDADAPTER_TESTER_HPP

#include "OBC/Components/UhfCcsdsVcidAdapter/UhfCcsdsVcidAdapter.hpp"
#include "OBC/Components/UhfCcsdsVcidAdapter/UhfCcsdsVcidAdapterGTestBase.hpp"

namespace OBC {

class UhfCcsdsVcidAdapterTester final : public UhfCcsdsVcidAdapterGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 8;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    UhfCcsdsVcidAdapterTester();
    ~UhfCcsdsVcidAdapterTester() override;

    void testDataPathStampsUhfVcid();
    void testReturnPathPreservesContext();
    void testStatusPathPassesThrough();

  private:
    void connectPorts();
    void initComponents();

  private:
    UhfCcsdsVcidAdapter component;
};

}  // namespace OBC

#endif
