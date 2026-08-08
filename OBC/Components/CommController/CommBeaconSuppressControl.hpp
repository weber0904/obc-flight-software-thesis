#ifndef OBC_COMMBEACONSUPPRESSCONTROL_HPP
#define OBC_COMMBEACONSUPPRESSCONTROL_HPP

namespace OBC {

class ICommBeaconSuppressControl {
  public:
    virtual ~ICommBeaconSuppressControl() = default;

    virtual void setUhfBeaconSuppressedForRuntime(bool suppressed) = 0;
};

}  // namespace OBC

#endif
