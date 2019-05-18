#ifndef BASE_MONITOR_H_
#define BASE_MONITOR_H_
#include <Auto.h>
#include <Type.h>
#include <Logcat.h>

#if __cplusplus
namespace Base {
class Monitor {
 public:
  enum TypeE {
    EIOSync = 0,
    EPipe = 1,
    ETimer = 2
  };

  using Check = Function<ErrorCodeE(Auto, Auto&, Int)>;
  using Perform = Function<ErrorCodeE(Auto, Auto&)>;
  using Indicate = Function<Perform*(Auto&, Int)>;
  using Fallback = Function<ErrorCodeE(Auto fd)>;

  /* @NOTE: these methods are used to append, modify or remove a fd */
  ErrorCodeE Append(Auto fd, Int mode);
  ErrorCodeE Modify(Auto fd, Int mode);
  ErrorCodeE Find(Auto fd);
  ErrorCodeE Remove(Auto fd);

  /* @NOTE: this method is used register a trigger when fd passes a
   * certain condition */
  ErrorCodeE Trigger(Auto event, Perform perform);
  void Heartbeat(Fallback fallback);

  /* @NOTE: this method is used to check status of the system */
  ErrorCodeE Status(String name);

  /* @NOTE: these methos are use to start monitor to run a certain step */
  ErrorCodeE Loop(Function<Bool(Monitor&)> status, Int timeout = 0);
  ErrorCodeE Loop(UInt steps = 1, Int timeout = 0);

  /* @NOTE: this method is call by HEAD monitor to probe fd's events */
  ErrorCodeE Scan(Auto fd, Int mode, Vector<Perform*>& callbacks);

  /* @NOTE: this function is the only way to create a monitor. With this
   * design, there is no way to create a monitor without diferent type */
  static Shared<Monitor> Make(String name, Monitor::TypeE type);

 protected:
  /* @NOTE: this function is used to registry an indicator which is used 
   * to generate a Perform. By default, indicator is always activated when
   * its checker accepts to open a route. When it happens, we will receive
   * a callback Perform and we will use Reroute to redirect the callback to
   * a place for performing */
  void Registry(Check check, Indicate indicate);

  /* @NOTE: this function is used to reroute a callback to specific place for
   * calling. How to handle these callbacks? how to keep balancing these
   * callbacks? most of them has been managed by Monitor and i will provide
   * convenient virtual methods to configure them */
  ErrorCodeE Reroute(Monitor* child, Auto fd, Perform& callback);

  /* @NOTE: this function is used to check if the fd is on fallback state which
   * lead to kill and remove its tasks */
  ErrorCodeE Heartbeat(Auto fd);

  /* @NOTE: acces the head monitor, they are defined as a single tone and should
   * be the latest remover */
  static Monitor* Head(TypeE type);

  /* @NOTE: these virtual methods are used to interact with monitor */
  virtual ErrorCodeE _Append(Auto fd, Int mode) = 0;
  virtual ErrorCodeE _Modify(Auto fd, Int mode) = 0;
  virtual ErrorCodeE _Find(Auto fd) = 0;
  virtual ErrorCodeE _Remove(Auto fd) = 0;

  /* @NOTE: this virtual method is used to register a triggering with
   * specific events */
  virtual ErrorCodeE _Trigger(Auto event, Perform perform) = 0;

  /* @NOTE: this virtual method is used to check status of system or a specific
   * fd whihch is installed into the monitor */
  virtual ErrorCodeE _Status(String name, Auto fd) = 0;

  /* @NOTE: this virtual method is used to interact with lowlevel */
  virtual ErrorCodeE _Interact(Monitor* child, Int timeout,
                               Int backlog = 100) = 0;

  /* @NOTE: this virtual method is used to access context of fd */
  virtual Auto& _Access(Auto fd) = 0;

  /* @NOTE: by default we will lock constructors and destructor to prevent
   * user to create a new monitor with uncompatible type */
  explicit Monitor(String name, TypeE type);
  virtual ~Monitor();

  /* @NOTE: these provide basic information of this monitor */
  String _Name;
  TypeE _Type;

  /* @NOTE: these should be managed by CHILD */
  Vector<Check> _Checks;
  Vector<Fallback> _Fallbacks;
  Map<ULong, Indicate> _Indicators;

  /* @NOTE: this should be managed by HEAD */
  Vector<Monitor*> _Children;
};
} // namespace Base
#else
#endif // __cplusplus
#endif // BASE_MONITOR_H_
